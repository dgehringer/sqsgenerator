//
// Created by Dominik Gehringer on 20.11.24.
//

#include <gtest/gtest.h>

#include "helpers.h"
#include "nlohmann/json.hpp"
#include "sqsgen/core/config.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/config/combined.h"
#include "sqsgen/io/json.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using json = nlohmann::json;
  using namespace sqsgen::io;
  using namespace sqsgen::io::config;

  template <class T> void assert_structure_equal(core::structure<T> const& lhs,
                                                 core::structure<T> const& rhs,
                                                 T epsilon = 1.0e-7) {
    ASSERT_EQ(lhs.size(), rhs.size()) << "Size of lhs and rhs are not equal";
    auto lhs_sites = core::helpers::as<std::vector>{}(lhs.sites());
    auto rhs_sites = core::helpers::as<std::vector>{}(rhs.sites());
    for (int i = 0; i < lhs.size(); ++i) {
      auto lsite = lhs_sites[i];
      auto rsite = rhs_sites[i];
      ASSERT_EQ(lsite.specie, rsite.specie);
      ASSERT_NEAR(lsite.frac_coords(0), rsite.frac_coords(0), epsilon);
      ASSERT_NEAR(lsite.frac_coords(1), rsite.frac_coords(1), epsilon);
      ASSERT_NEAR(lsite.frac_coords(2), rsite.frac_coords(2), epsilon);
    }
  }

  template <class T> const static auto TEST_FCC_STRUCTURE = core::structure<T>{
      lattice_t<T>{{1, 0, 0}, {0, 2, 0}, {0, 0, 3}},
      coords_t<T>{{0.0, 0.0, 0.0}, {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0}},
      std::vector<specie_t>{11, 12, 13, 14}};

  template <class T>
  static auto make_test_structure_config(std::optional<std::array<int, 3>> supercell
                                         = std::nullopt) {
    auto default_supercell = std::array<int, 3>({1, 1, 1});
    return nlohmann::json{{"structure",
                           {{"lattice", TEST_FCC_STRUCTURE<T>.lattice},
                            {"coords", TEST_FCC_STRUCTURE<T>.frac_coords},
                            {"species", TEST_FCC_STRUCTURE<T>.species},
                            {"supercell", supercell.value_or(default_supercell)}}}};
  }

  template <class T>
  static auto make_test_structure_and_composition(std::optional<std::array<int, 3>> supercell
                                                  = std::nullopt) {
    auto [a, b, c] = supercell.value_or(std::array{1, 1, 1});
    auto n = a * b * c;
    auto json = make_test_structure_config<T>(std::move(supercell));
    json["composition"] = nlohmann::json{{"Ni", 2 * n}, {"Co", 2 * n}};
    return json;
  }

  template <class T> static auto make_test_structure_and_composition_multiple(
      std::optional<std::array<int, 3>> supercell = std::nullopt) {
    auto [a, b, c] = supercell.value_or(std::array{1, 1, 1});
    auto n = a * b * c;
    auto json = make_test_structure_config<T>(std::move(supercell));
    json["composition"]
        = nlohmann::json{{{"Ni", n}, {"Co", n}, {"sites", {"Al", "Mg"}}}, {{"B", n}, {"N", n}}};
    return json;
  }

  template <class Fn> auto make_assert_holds_error(json& j, Fn&& fn) {
    return [&](std::string const& key, parse_error_code jcode,
               std::optional<parse_error_code> dcode = std::nullopt) {
      auto rjson = fn(j);
      ASSERT_TRUE(rjson.failed());
      parse_error error = rjson.error();
      ASSERT_EQ(key, error.key) << error.msg;
      ASSERT_EQ(jcode, error.code) << error.msg;
    };
  }

  TEST(test_parse_structure, empty) {
    auto rjson = io::config::parse_structure_config<"composition", double>(nlohmann::json{});
    ASSERT_TRUE(rjson.failed());
  }

  TEST(test_parse_structure, DISABLED_required_fields_success) {
    using namespace py::literals;
    auto s = TEST_FCC_STRUCTURE<double>;
    json json = {
        {"structure", {{"lattice", s.lattice}, {"coords", s.frac_coords}, {"species", s.species}}}};

    auto rjson = io::config::parse_structure_config<"structure", double>(json);

    assert_structure_equal(s, rjson.result().structure());

    json["structure"]["species"] = std::vector<std::string>{"Na", "Mg", "Al", "Si"};
    rjson = io::config::parse_structure_config<"structure", double>(json);
    ASSERT_TRUE(rjson.ok());
    assert_structure_equal(s, rjson.result().structure());
  }

  TEST(test_parse_structure, required_fields_errors) {
    auto json = make_test_structure_config<double>();
    json["structure"]["species"] = 4;

    auto parse = []<class Doc>(Doc const& doc) {
      return io::config::parse_structure_config<"structure", double>(doc);
    };
    auto assert_holds_error = make_assert_holds_error(json, parse);

    json["structure"].erase("species");
    assert_holds_error("species", CODE_NOT_FOUND);
    json["structure"]["species"] = {1, 2, 3};
    assert_holds_error("species", CODE_OUT_OF_RANGE);

    // test invalid atomic speices
    json["structure"]["species"] = {1, 2, 3, -5};
    assert_holds_error("species", CODE_OUT_OF_RANGE);

    json["structure"]["species"] = {"Al", "Mg", "Si", "??"};
    // test invalid atomic species
    assert_holds_error("species", CODE_OUT_OF_RANGE);

    json["structure"]["species"] = {"Al", "Mg", "Si", "Ge"};
    json["structure"]["coords"] = {1, 2, 3, 4};
    // test wrong shape for coords
    assert_holds_error("coords", CODE_TYPE_ERROR);

    // test wrong shape for lattice
    auto s = TEST_FCC_STRUCTURE<double>;
    json["structure"]["lattice"] = s.frac_coords;
    json["structure"]["coords"] = s.lattice;
    assert_holds_error("lattice", CODE_OUT_OF_RANGE, CODE_TYPE_ERROR);
  }

  TEST(test_parse_composition, error_species) {
    auto json = make_test_structure_config<double>();
    auto key = "composition";

    auto parse_composition
        = []<class Doc>(Doc const& doc) -> parse_result<std::vector<sublattice>> {
      auto structure = config::parse_structure_config<"structure", double, Doc>(doc);
      return config::parse_composition<"composition", "sites", Doc>(doc, structure.result().species,
                                                                    SUBLATTICE_MODE_INTERACT);
    };

    auto assert_holds_error = make_assert_holds_error(json, parse_composition);
    assert_holds_error(key, CODE_NOT_FOUND);

    json[key] = json::array();
    assert_holds_error(key, CODE_OUT_OF_RANGE);

    json[key] = {{"Ni", -1}};
    assert_holds_error(key, CODE_BAD_VALUE);

    // enclosing in list must result in same error
    json[key] = {{{"Ni", -1}}};
    assert_holds_error(key, CODE_BAD_VALUE);

    json[key] = {{"Ni", "1"}};
    assert_holds_error(key, CODE_TYPE_ERROR);

    // enclosing in list must result in same error
    json[key] = {{{"Ni", "1"}}};
    assert_holds_error(key, CODE_TYPE_ERROR);

    json[key] = {{"_Ni", "1"}};
    assert_holds_error(key, CODE_OUT_OF_RANGE);

    // enclosing in list must result in same error
    json[key] = {{{"_Ni", "1"}}};
    assert_holds_error(key, CODE_OUT_OF_RANGE);

    json[key] = {{"Ni", 3}, {"Fe", 2}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // enclosing in list must result in same error
    json[key] = {{{"Ni", 3}, {"Fe", 2}}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE);
  }

  TEST(test_parse_composition, error_sites) {
    auto json = make_test_structure_config<double>();

    auto assert_holds_error = make_assert_holds_error(
        json, []<class Doc>(Doc const& doc) -> parse_result<std::vector<sublattice>> {
          auto structure = config::parse_structure_config<"structure", double, Doc>(doc);
          return config::parse_composition<"composition", "sites", Doc>(
              doc, structure.result().species, SUBLATTICE_MODE_INTERACT);
        });
    auto key = "composition";

    assert_holds_error(key, CODE_NOT_FOUND);

    // test invalid index
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", {-1, 2}}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE);
    // test invalid indices empty
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", std::vector<int>{}}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // test too few sites
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", {0, 2}}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // test species not in structure
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", "Fr"}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // test too few sites - by symbol
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", {"Si", "Mg"}}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // test too many sites - by symbol -> invalid symbol
    json[key] = {{"Ni", 1}, {"_Co", 1}, {"sites", {"Si", "Mg"}}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE);
  }

  TEST(test_parse_composition, error_multiple) {
    auto json = make_test_structure_config<double>();

    auto assert_holds_error = make_assert_holds_error(
        json, []<class Doc>(Doc const& doc) -> parse_result<std::vector<sublattice>> {
          auto structure = config::parse_structure_config<"structure", double, Doc>(doc);
          return config::parse_composition<"composition", "sites", Doc>(
              doc, structure.result().species, SUBLATTICE_MODE_SPLIT);
        });
    auto key = "composition";
    assert_holds_error(key, CODE_NOT_FOUND);

    // test invalid index
    nlohmann::json sl1 = {{"Ni", 1}, {"Co", 1}, {"sites", {0, 1}}};
    nlohmann::json sl2 = {{"Ni", 1}, {"Co", 1}, {"sites", {1, 2}}};
    json[key] = {sl1, sl2};
    assert_holds_error("sites", CODE_BAD_VALUE);

    // overlap by species array
    json[key][1]["sites"] = {"Na", "Si"};
    assert_holds_error("sites", CODE_BAD_VALUE);

    // sublattice overlap by species definition
    json[key][1]["sites"] = "Mg";
    assert_holds_error("sites", CODE_BAD_VALUE);

    // create a sublattice with three Ni:2, Co: 1 atoms and have only two sites lift
    json[key][1].erase("sites");
    json[key][1]["Ni"] = 2;
    assert_holds_error("sites", CODE_OUT_OF_RANGE);
  }

  template <class Doc>
  parse_result<stl_matrix_t<double>> parse_radii(Doc const& doc, SublatticeMode mode) {
    using result_t = parse_result<stl_matrix_t<double>>;
    return config::parse_structure_config<"structure", double, Doc>(doc).and_then(
        [&](auto&& sc) -> result_t {
          auto structure = sc.structure();
          return config::parse_composition<"composition", "sites", Doc>(doc, structure.species,
                                                                        mode)
              .and_then([&](auto&& composition) -> result_t {
                return config::parse_shell_radii<"shell_radii">(
                    doc, mode, std::forward<decltype(structure)>(structure), composition);
              });
        });
  }

  TEST(test_parse_shell_radii, default_case) {
    using namespace sqsgen::io;
    auto module = py::module::import("ShellRadiiDetection");

    auto json = make_test_structure_and_composition<double>(std::array{2, 2, 2});

    auto rdefault_json = parse_radii(json, SUBLATTICE_MODE_INTERACT);
    ASSERT_TRUE(rdefault_json.ok());

    json["shell_radii"] = "peak";
    auto rpeak_json = parse_radii(json, SUBLATTICE_MODE_INTERACT);

    ASSERT_TRUE(rpeak_json.ok());
  }

  TEST(test_parse_shell_radii, perfect_lattice) {
    using namespace sqsgen::io;
    auto module = py::module::import("ShellRadiiDetection");

    auto json = make_test_structure_and_composition<double>(std::array{2, 2, 2});
    json["bin_width"] = 0.001;
    auto rdefault_json = parse_radii(json, SUBLATTICE_MODE_INTERACT);

    json["shell_radii"] = "naive";
    auto rnaive_json = parse_radii(json, SUBLATTICE_MODE_INTERACT);

    // For a perfect lattice it does not matter whether we use naive or histogram method
    // assuming the bin_width is small enough
    helpers::assert_vector_equal(rdefault_json.result(), rnaive_json.result());
  }

  TEST(test_parse_shell_radii, perfect_lattice_multiple_sublattices) {
    using namespace sqsgen::io;
    auto json = make_test_structure_and_composition_multiple<double>(std::array{2, 2, 2});
    json["sublattice_mode"] = "split";
    json["bin_width"] = 0.001;

    auto rdefault_json = parse_radii(json, SUBLATTICE_MODE_SPLIT);
    json["shell_radii"] = {"naive", "naive"};
    auto rnaive_json = parse_radii(json, SUBLATTICE_MODE_SPLIT);

    // For a perfect lattice it does not matter whether we use naive or histogram method
    // assuming the bin_width is small enough
    helpers::assert_vector_equal(rdefault_json.result(), rnaive_json.result());
  }

  TEST(test_parse_shell_weights, errors) {
    using namespace sqsgen::io;
    auto key = "shell_weights";

    auto json = make_test_structure_and_composition<double>();
    json[key] = {{"0", 0.0}};

    auto assert_holds_error = make_assert_holds_error(json, []<class Doc>(Doc const& doc) {
      auto radii = parse_radii(doc, SUBLATTICE_MODE_INTERACT);
      return config::parse_shell_weights<"shell_weights", double>(doc, SUBLATTICE_MODE_INTERACT,
                                                                  radii.result());
    });

    assert_holds_error("shell_weights", CODE_BAD_VALUE);

    json[key].erase("0");
    json[key]["200"] = 200.0;

    assert_holds_error("shell_weights", CODE_OUT_OF_RANGE);

    // we have interacting mode by default, therefore lists should result in a TYPE_ERROR
    nlohmann::json jshells{{"1", 1.0}, {"2", 2.0}};
    json[key] = {jshells, jshells};
    assert_holds_error("shell_weights", CODE_BAD_VALUE);
  }

  TEST(test_parse_shell_weights, weights_default) {
    using namespace sqsgen::io;
    auto json = make_test_structure_and_composition<double>(std::array{2, 2, 2});

    auto parse_weights = []<class Doc>(Doc const& doc, SublatticeMode mode) -> weights_t<double> {
      auto radii = parse_radii(doc, mode);
      return config::parse_shell_weights<"shell_weights", double>(doc, mode, radii.result());
    };

    auto rjson = parse_weights(json, SUBLATTICE_MODE_INTERACT);
    ASSERT_TRUE(rjson.ok());
    ASSERT_EQ(rjson.result().size(), 1);

    json = make_test_structure_and_composition_multiple<double>(std::array{2, 2, 2});
    json["sublattice_mode"] = "split";
    rjson = parse_weights(json, SUBLATTICE_MODE_SPLIT);
    ASSERT_TRUE(rjson.ok());
    ASSERT_EQ(rjson.result().size(), 2);
    shell_weights_t<double> w{{1, 1.0},       {2, 1.0 / 2.0}, {3, 1.0 / 3.0}, {4, 1.0 / 4.0},
                              {5, 1.0 / 5.0}, {6, 1.0 / 6.0}, {7, 1.0 / 7.0}, {8, 1.0 / 8.0}};
    ASSERT_EQ(rjson.result()[0], w);
    ASSERT_EQ(rjson.result()[1], w);
  }

  TEST(test_parse_arrays, prefactors_interact) {
    auto json = make_test_structure_and_composition_multiple<float>(std::array{3, 3, 3});

    auto rjson = config::parse_config_for_prec<double>(json);
    ASSERT_TRUE(rjson.ok());

    ASSERT_EQ(rjson.result().pair_weights.size(), 1);
    ASSERT_EQ(rjson.result().target_objective.size(), 1);
    ASSERT_EQ(rjson.result().prefactors.size(), 1);
  }
}  // namespace sqsgen::testing
