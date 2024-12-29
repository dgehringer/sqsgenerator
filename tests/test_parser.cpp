//
// Created by Dominik Gehringer on 20.11.24.
//

#include <gtest/gtest.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "helpers.h"
#include "nlohmann/json.hpp"
#include "sqsgen/config.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/config/composition.h"
#include "sqsgen/io/config/settings.h"
#include "sqsgen/io/config/shell_radii.h"
#include "sqsgen/io/config/structure.h"
#include "sqsgen/io/dict.h"
#include "sqsgen/io/json.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using json = nlohmann::json;
  using namespace sqsgen::io;
  using namespace sqsgen::io::config;
  namespace py = pybind11;
  using namespace py::literals;

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
  template <class... Args> py::list as_pylist(Args&&... obj) {
    py::list list;
    (list.append(py::handle(obj)), ...);
    return list;
  };

  template <class T> const static auto TEST_FCC_STRUCTURE = core::structure<T>{
      lattice_t<T>{{1, 0, 0}, {0, 2, 0}, {0, 0, 3}},
      coords_t<T>{{0.0, 0.0, 0.0}, {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0}},
      std::vector<specie_t>{11, 12, 13, 14}};

  template <class T>
  static auto make_test_structure_config(std::optional<std::array<int, 3>> supercell
                                         = std::nullopt) {
    auto default_supercell = std::array<int, 3>({1, 1, 1});
    return std::pair{
        nlohmann::json{{"structure",
                        {{"lattice", TEST_FCC_STRUCTURE<T>.lattice},
                         {"coords", TEST_FCC_STRUCTURE<T>.frac_coords},
                         {"species", TEST_FCC_STRUCTURE<T>.species},
                         {"supercell", supercell.value_or(default_supercell)}}}},
        py::dict("structure"_a = py::dict("lattice"_a = TEST_FCC_STRUCTURE<T>.lattice,
                                          "coords"_a = TEST_FCC_STRUCTURE<T>.frac_coords,
                                          "species"_a = TEST_FCC_STRUCTURE<T>.species,
                                          "supercell"_a = supercell.value_or(default_supercell)))};
  }

  template <class T>
  static auto make_test_structure_and_composition(std::optional<std::array<int, 3>> supercell
                                                  = std::nullopt) {
    auto [a, b, c] = supercell.value_or(std::array{1, 1, 1});
    auto n = a * b * c;
    auto [json, dict] = make_test_structure_config<T>(std::move(supercell));
    dict["composition"] = py::dict("Ni"_a = 2 * n, "Co"_a = 2 * n);
    json["composition"] = nlohmann::json{{"Ni", 2 * n}, {"Co", 2 * n}};
    return std::make_pair(json, dict);
  }

  template <class T> static auto make_test_structure_and_composition_multiple(
      std::optional<std::array<int, 3>> supercell = std::nullopt) {
    auto [a, b, c] = supercell.value_or(std::array{1, 1, 1});
    auto n = a * b * c;
    auto [json, dict] = make_test_structure_config<T>(std::move(supercell));

    dict["composition"]
        = as_pylist(py::dict("Ni"_a = n, "Co"_a = n, "sites"_a = std::vector{"Al", "Mg"}),
                    py::dict("B"_a = n, "N"_a = n));
    json["composition"]
        = nlohmann::json{
          {{"Ni", n}, {"Co", n}, {"sites", {"Al", "Mg"}}}, {{"B", n}, {"N", n}}};
    return std::make_pair(json, dict);
  }

  template <class Fn> auto make_assert_holds_error(json& j, py::dict& d, Fn&& fn) {
    return [&](std::string const& key, parse_error_code jcode,
               std::optional<parse_error_code> dcode = std::nullopt) {
      auto rjson = fn(j);
      auto rdict = fn(py::handle(d));
      ASSERT_TRUE(rjson.failed());
      ASSERT_TRUE(rdict.failed());
      parse_error error = rjson.error();
      ASSERT_EQ(key, error.key) << error.msg;
      ASSERT_EQ(jcode, error.code) << error.msg;
      auto dict_code = dcode.value_or(jcode);
      ASSERT_EQ(dict_code, rdict.error().code) << rdict.error().msg;
    };
  }

  TEST(test_parse_structure, empty) {
    auto rjson = io::config::parse_structure_config<"composition", double>(nlohmann::json{});
    ASSERT_TRUE(rjson.failed());
  }

  TEST(test_parse_structure, required_fields_success) {
    using namespace py::literals;
    py::scoped_interpreter guard{};
    auto s = TEST_FCC_STRUCTURE<double>;
    json json = {
        {"structure", {{"lattice", s.lattice}, {"coords", s.frac_coords}, {"species", s.species}}}};
    py::dict dict("structure"_a = py::dict("lattice"_a = s.lattice, "coords"_a = s.frac_coords,
                                           "species"_a = s.species));

    auto rjson = io::config::parse_structure_config<"structure", double>(json);
    auto rdict = io::config::parse_structure_config<"structure", double>(py::handle(dict));

    assert_structure_equal(s, rjson.result().structure());
    assert_structure_equal(rdict.result().structure(), rjson.result().structure());

    json["structure"]["species"] = std::vector<std::string>{"Na", "Mg", "Al", "Si"};
    dict["structure"]["species"] = std::vector<std::string>{"Na", "Mg", "Al", "Si"};
    rjson = io::config::parse_structure_config<"structure", double>(json);
    rdict = io::config::parse_structure_config<"structure", double>(py::handle(dict));
    ASSERT_TRUE(rjson.ok());
    ASSERT_TRUE(rdict.ok());
    assert_structure_equal(s, rjson.result().structure());
    assert_structure_equal(rdict.result().structure(), rjson.result().structure());
  }

  TEST(test_parse_structure, required_fields_errors) {
    py::scoped_interpreter guard{};

    auto [json, dict] = make_test_structure_config<double>();
    json["structure"]["species"] = 4;
    dict["structure"]["species"] = 4;

    auto parse = []<class Doc>(Doc const& doc) {
      return io::config::parse_structure_config<"structure", double>(doc);
    };
    auto assert_holds_error = make_assert_holds_error(json, dict, parse);

    json["structure"].erase("species");
    dict["structure"].attr("pop")("species");
    assert_holds_error("species", CODE_NOT_FOUND);
    json["structure"]["species"] = {1, 2, 3};
    dict["structure"]["species"] = std::vector{1, 2, 3};
    assert_holds_error("species", CODE_OUT_OF_RANGE);

    // test invalid atomic speices
    json["structure"]["species"] = {1, 2, 3, -5};
    dict["structure"]["species"] = std::vector{1, 2, 3, -5};
    assert_holds_error("species", CODE_OUT_OF_RANGE);

    json["structure"]["species"] = {"Al", "Mg", "Si", "??"};
    dict["structure"]["species"] = std::vector{"Al", "Mg", "Si", "??"};
    // test invalid atomic species
    assert_holds_error("species", CODE_OUT_OF_RANGE);

    json["structure"]["species"] = {"Al", "Mg", "Si", "Ge"};
    json["structure"]["coords"] = {1, 2, 3, 4};
    dict["structure"]["species"] = std::vector{"Al", "Mg", "Si", "Ge"};
    dict["structure"]["coords"] = std::vector{1, 2, 3, 4};
    // test wrong shape for coords
    assert_holds_error("coords", CODE_TYPE_ERROR);

    // test wrong shape for lattice
    auto s = TEST_FCC_STRUCTURE<double>;
    json["structure"]["lattice"] = s.frac_coords;
    json["structure"]["coords"] = s.lattice;
    dict["structure"]["lattice"] = s.frac_coords;
    dict["structure"]["coords"] = s.lattice;
    assert_holds_error("lattice", CODE_OUT_OF_RANGE, CODE_TYPE_ERROR);
  }

  TEST(test_parse_composition, error_species) {
    py::scoped_interpreter guard{};
    auto [json, dict] = make_test_structure_config<double>();
    auto key = "composition";

    auto parse_composition
        = []<class Doc>(Doc const& doc) -> parse_result<std::vector<sublattice>> {
      auto structure = config::parse_structure_config<"structure", double, Doc>(doc);
      return config::parse_composition<"composition", "sites", Doc>(doc,
                                                                    structure.result().species);
    };

    auto assert_holds_error = make_assert_holds_error(json, dict, parse_composition);
    assert_holds_error(key, CODE_NOT_FOUND);

    json[key] = json::array();
    dict[key] = py::list();
    assert_holds_error(key, CODE_OUT_OF_RANGE);

    json[key] = {{"Ni", -1}};
    dict[key] = py::dict("Ni"_a = -1);
    assert_holds_error(key, CODE_BAD_VALUE);

    // enclosing in list must result in same error
    json[key] = {{{"Ni", -1}}};
    dict[key] = as_pylist(py::dict("Ni"_a = -1));
    assert_holds_error(key, CODE_BAD_VALUE);

    json[key] = {{"Ni", "1"}};
    dict[key] = py::dict("Ni"_a = "1");
    assert_holds_error(key, CODE_TYPE_ERROR);

    // enclosing in list must result in same error
    json[key] = {{{"Ni", "1"}}};
    dict[key] = as_pylist(py::dict("Ni"_a = "1"));
    assert_holds_error(key, CODE_TYPE_ERROR);

    json[key] = {{"_Ni", "1"}};
    dict[key] = py::dict("_Ni"_a = "1");
    assert_holds_error(key, CODE_OUT_OF_RANGE);

    // enclosing in list must result in same error
    json[key] = {{{"_Ni", "1"}}};
    dict[key] = as_pylist(py::dict("_Ni"_a = "1"));
    assert_holds_error(key, CODE_OUT_OF_RANGE);

    json[key] = {{"Ni", 3}, {"Fe", 2}};
    dict[key] = py::dict("Ni"_a = 3, "Fe"_a = 2);
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // enclosing in list must result in same error
    json[key] = {{{"Ni", 3}, {"Fe", 2}}};
    dict[key] = as_pylist(py::dict("Ni"_a = 3, "Fe"_a = 2));
    assert_holds_error("sites", CODE_OUT_OF_RANGE);
  }

  TEST(test_parse_composition, error_sites) {
    py::scoped_interpreter guard{};
    auto [json, dict] = make_test_structure_config<double>();

    auto assert_holds_error = make_assert_holds_error(
        json, dict, []<class Doc>(Doc const& doc) -> parse_result<std::vector<sublattice>> {
          auto structure = config::parse_structure_config<"structure", double, Doc>(doc);
          return config::parse_composition<"composition", "sites", Doc>(doc,
                                                                        structure.result().species);
        });
    auto key = "composition";

    assert_holds_error(key, CODE_NOT_FOUND);

    // test invalid index
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", {-1, 2}}};
    dict[key] = py::dict("Ni"_a = 2, "Co"_a = 2, "sites"_a = std::vector{-1, 2});
    assert_holds_error("sites", CODE_OUT_OF_RANGE);
    // test invalid indices empty
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", std::vector<int>{}}};
    dict[key] = py::dict("Ni"_a = 2, "Co"_a = 2, "sites"_a = py::list());
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // test too few sites
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", {0, 2}}};
    dict[key] = py::dict("Ni"_a = 2, "Co"_a = 2, "sites"_a = std::vector{0, 2});
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // test species not in structure
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", "Fr"}};
    dict[key] = py::dict("Ni"_a = 2, "Co"_a = 2, "sites"_a = "Fr");
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // test too few sites - by symbol
    json[key] = {{"Ni", 2}, {"Co", 2}, {"sites", {"Si", "Mg"}}};
    dict[key] = py::dict("Ni"_a = 2, "Co"_a = 2, "sites"_a = std::vector{"Si", "Mg"});
    assert_holds_error("sites", CODE_OUT_OF_RANGE);

    // test too many sites - by symbol -> invalid symbol
    json[key] = {{"Ni", 1}, {"_Co", 1}, {"sites", {"Si", "Mg"}}};
    dict[key] = py::dict("Ni"_a = 1, "_Co"_a = 1, "sites"_a = std::vector{"Si", "Mg"});
    assert_holds_error("sites", CODE_OUT_OF_RANGE);
  }
  TEST(test_parse_composition, error_multiple) {
    py::scoped_interpreter guard{};
    auto [json, dict] = make_test_structure_config<double>();

    auto assert_holds_error = make_assert_holds_error(
        json, dict, []<class Doc>(Doc const& doc) -> parse_result<std::vector<sublattice>> {
          auto structure = config::parse_structure_config<"structure", double, Doc>(doc);
          return config::parse_composition<"composition", "sites", Doc>(doc,
                                                                        structure.result().species);
        });
    auto key = "composition";
    assert_holds_error(key, CODE_NOT_FOUND);

    // test invalid index
    nlohmann::json sl1 = {{"Ni", 1}, {"Co", 1}, {"sites", {0, 1}}};
    nlohmann::json sl2 = {{"Ni", 1}, {"Co", 1}, {"sites", {1, 2}}};
    json[key] = {sl1, sl2};

    dict[key] = as_pylist(py::dict("Ni"_a = 1, "Co"_a = 1, "sites"_a = std::vector{0, 1}),
                          py::dict("Ni"_a = 1, "Co"_a = 1, "sites"_a = std::vector{1, 2}));
    assert_holds_error("sites", CODE_BAD_VALUE);

    // overlap by species array
    json[key][1]["sites"] = {"Na", "Si"};
    dict[key].cast<py::list>()[1]["sites"] = std::vector{"Na", "Si"};
    assert_holds_error("sites", CODE_BAD_VALUE);

    // sublattice overlap by species definition
    json[key][1]["sites"] = "Mg";
    dict[key].cast<py::list>()[1]["sites"] = "Mg";
    assert_holds_error("sites", CODE_BAD_VALUE);

    // create a sublattice with three Ni:2, Co: 1 atoms and have only two sites lift
    json[key][1].erase("sites");
    json[key][1]["Ni"] = 2;
    dict[key].cast<py::list>()[1].attr("pop")("sites");
    dict[key].cast<py::list>()[1]["Ni"] = 2;
    assert_holds_error("sites", CODE_OUT_OF_RANGE);
  }

  PYBIND11_EMBEDDED_MODULE(ShellRadiiDetection, m) {
    py::enum_<ShellRadiiDetection>(m, "ShellRadiiDetection")
        .value("naive", SHELL_RADII_DETECTION_NAIVE)
        .value("peak", SHELL_RADII_DETECTION_PEAK)
        .export_values();

    py::enum_<SublatticeMode>(m, "SublatticeMode")
        .value("interact", SUBLATTIC_MODE_INTERACT)
        .value("split", SUBLATTIC_MODE_SPLIT)
        .export_values();
  }

  template <class Doc> parse_result<stl_matrix_t<double>> parse_radii(Doc const& doc) {
    using result_t = parse_result<stl_matrix_t<double>>;
    return config::parse_structure_config<"structure", double, Doc>(doc).and_then(
        [&](auto&& sc) -> result_t {
          auto structure = sc.structure();
          return config::parse_composition<"composition", "sites", Doc>(doc, structure.species)
              .and_then([&](auto&& composition) -> result_t {
                return config::parse_shell_radii<"shell_radii">(
                    doc, std::forward<decltype(structure)>(structure), composition);
              });
        });
  };

  TEST(test_parse_shell_radii, default_case) {
    using namespace sqsgen::io;
    py::scoped_interpreter guard{};
    auto module = py::module::import("ShellRadiiDetection");

    auto [json, dict] = make_test_structure_and_composition<double>(std::array{2, 2, 2});

    auto rdefault_dict = parse_radii(py::handle(dict));
    auto rdefault_json = parse_radii(json);
    ASSERT_TRUE(rdefault_dict.ok());
    ASSERT_TRUE(rdefault_json.ok());

    json["shell_radii"] = "peak";
    dict["shell_radii"] = module.attr("ShellRadiiDetection").attr("peak");
    auto rpeak_dict = parse_radii(py::handle(dict));
    auto rpeak_json = parse_radii(json);

    ASSERT_TRUE(rpeak_dict.ok());
    ASSERT_TRUE(rpeak_json.ok());

    helpers::assert_vector_equal(rdefault_dict.result(), rpeak_dict.result());
    helpers::assert_vector_equal(rdefault_json.result(), rpeak_json.result());
  }

  TEST(test_parse_shell_radii, perfect_lattice) {
    using namespace sqsgen::io;
    py::scoped_interpreter guard{};
    auto module = py::module::import("ShellRadiiDetection");

    auto [json, dict] = make_test_structure_and_composition<double>(std::array{2, 2, 2});
    json["bin_width"] = 0.001;
    dict["bin_width"] = 0.001;
    auto rdefault_dict = parse_radii(py::object(dict));
    auto rdefault_json = parse_radii(json);

    json["shell_radii"] = "naive";
    dict["shell_radii"] = module.attr("ShellRadiiDetection").attr("naive");
    auto rnaive_dict = parse_radii(py::object(dict));
    auto rnaive_json = parse_radii(json);

    // For a perfect lattice it does not matter whether we use naive or histogram method
    // assuming the bin_width is small enough
    helpers::assert_vector_equal(rdefault_dict.result(), rnaive_dict.result());
    helpers::assert_vector_equal(rdefault_json.result(), rnaive_json.result());
  }

  TEST(test_parse_shell_radii, perfect_lattice_multiple_sublattices) {
    using namespace sqsgen::io;
    py::scoped_interpreter guard{};
    auto module = py::module::import("ShellRadiiDetection");

    auto [json, dict] = make_test_structure_and_composition_multiple<double>(std::array{2, 2, 2});
    json["sublattice_mode"] = "split";
    dict["sublattice_mode"] = module.attr("SublatticeMode").attr("split");
    json["bin_width"] = 0.001;
    dict["bin_width"] = 0.001;

    auto rdefault_dict = parse_radii(py::handle(dict));
    auto rdefault_json = parse_radii(json);
    auto naive_mode = module.attr("ShellRadiiDetection").attr("naive");
    // auto peak_mode = module.attr("ShellRadiiDetection").attr("peak");
    json["shell_radii"] = {"naive", "naive"};
    dict["shell_radii"] = as_pylist(naive_mode, naive_mode);
    auto rnaive_dict = parse_radii(py::handle(dict));
    auto rnaive_json = parse_radii(json);

    // For a perfect lattice it does not matter whether we use naive or histogram method
    // assuming the bin_width is small enough
    helpers::assert_vector_equal(rdefault_dict.result(), rnaive_dict.result());
    helpers::assert_vector_equal(rdefault_json.result(), rnaive_json.result());
  }

  TEST(test_parse_shell_weights, errors) {
    using namespace sqsgen::io;
    py::scoped_interpreter guard{};
    auto key = "shell_weights";

    auto [json, dict] = make_test_structure_config<double>();
    json[key] = {};
    dict[key] = py::dict();
    dict[key][py::int_(0)] = 0;

    /*auto assert_holds_error = make_assert_holds_error(
        json, dict, []<class Doc>(Doc const& doc) -> parse_result<shell_weights_t<double>> {
          auto structure = config::parse_structure_config<"structure", double, Doc>(doc);
          auto s = structure.result().structure();
          auto composition = config::parse_composition<"composition", "sites">(doc, s.species);
          auto radii = parse_shell_radii<"shell_radii", double>(doc, std::forward<decltype(s)>(s),
                                                                composition.result());
          auto num_shells = radii.result().size();
          return config::parse_weights<"shell_weights", double>(doc, num_shells);
        });*/

    // assert_holds_error("shell_weights", CODE_BAD_VALUE);
  }
  /*
    TEST(test_parse_shell_weights, errors) {
      auto s = TEST_FCC_STRUCTURE<double>;
      json document{{"structure",
                     {{"lattice", s.lattice},
                      {"coords", s.frac_coords},
                      {"species", s.species},
                      {"supercell", {2, 2, 2}}}}};
      ASSERT_EQ(parse_structure_config<"structure", double>(document).result().structure().size(),
                32);

    }
  */
}  // namespace sqsgen::testing