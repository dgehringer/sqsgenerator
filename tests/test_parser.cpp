//
// Created by Dominik Gehringer on 20.11.24.
//

#include <gtest/gtest.h>

#include "helpers.h"
#include "nlohmann/json.hpp"
#include "sqsgen/config.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/config/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using json = nlohmann::json;
  using namespace sqsgen::io::config;

  template <class Fn> auto make_assert_holds_error(Fn&& fn) {
    return [&](std::string const& key, parse_error_code code, nlohmann::json const& a) {
      auto r = fn(a);
      ASSERT_TRUE(holds_error(r));
      parse_error error = std::get<parse_error>(r);
      ASSERT_EQ(key, error.key) << error.msg;
      ASSERT_EQ(code, error.code) << error.msg;
    };
  }

  template <class T> const static auto TEST_FCC_STRUCTURE = core::structure<T>{
      lattice_t<T>{{1, 0, 0}, {0, 2, 0}, {0, 0, 3}},
      coords_t<T>{{0.0, 0.0, 0.0}, {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0}},
      std::vector<specie_t>{11, 12, 13, 14}};

  TEST(test_parse_structure, empty) {
    ASSERT_TRUE(holds_error(io::config::parse_structure_config<"composition", double>({})));
  }

  TEST(test_parse_structure, required_fields_success) {
    auto s = TEST_FCC_STRUCTURE<double>;
    json document = {{"structure",
                      {
                          {"lattice", s.lattice},
                          {"coords", s.frac_coords},
                          {"species", s.species},
                      }}};

    auto r1 = io::config::parse_structure_config<"structure", double>(document);

    ASSERT_TRUE(holds_result(r1));
    helpers::assert_structure_equal(s, get_result(r1).structure());

    document["structure"]["species"] = std::vector<std::string>{"Na", "Mg", "Al", "Si"};
    auto r2 = io::config::parse_structure_config<"structure", double>(document);
    ASSERT_TRUE(holds_result(r2));
    helpers::assert_structure_equal(s, get_result(r2).structure());
  }

  TEST(test_parse_structure, required_fields_errors) {
    auto s = TEST_FCC_STRUCTURE<double>;
    json document = {{"structure",
                      {
                          {"lattice", s.lattice},
                          {"coords", s.frac_coords},
                          {"species", 4},
                      }}};

    auto assert_holds_error
        = make_assert_holds_error(io::config::parse_structure_config<"structure", double>);

    document["structure"].erase("species");
    assert_holds_error("species", CODE_NOT_FOUND, document);
    document["structure"]["species"] = {1, 2, 3};
    assert_holds_error("species", CODE_OUT_OF_RANGE, document);

    // test invalid atomic speices
    document["structure"]["species"] = {1, 2, 3, -5};
    assert_holds_error("species", CODE_OUT_OF_RANGE, document);

    document["structure"]["species"] = {"Al", "Mg", "Si", "??"};
    // test invalid atomic species
    assert_holds_error("species", CODE_OUT_OF_RANGE, document);

    document["structure"]["species"] = {"Al", "Mg", "Si", "Ge"};
    document["structure"]["coords"] = std::vector{1, 2, 3, 4};
    // test wrong shape for coords
    assert_holds_error("coords", CODE_TYPE_ERROR, document);

    // test wrong shape for lattice
    document["structure"]["lattice"] = s.frac_coords;
    document["structure"]["coords"] = s.lattice;
    assert_holds_error("lattice", CODE_OUT_OF_RANGE, document);
  }

  TEST(test_parse_composition, error_species) {
    auto s = TEST_FCC_STRUCTURE<double>;
    json document{
        {"structure", {{"lattice", s.lattice}, {"coords", s.frac_coords}, {"species", s.species}}}};
    auto key = "composition";

    auto parse_composition = [](json const& doc) -> parse_result_t<std::vector<sublattice>> {
      auto structure = io::config::parse_structure_config<"structure", double>(doc);
      return io::config::parse_composition<"composition">(doc, get_result(structure).species);
    };

    auto assert_holds_error = make_assert_holds_error(parse_composition);
    assert_holds_error(key, CODE_NOT_FOUND, document);

    document[key] = json::array();
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);

    document[key] = {{"Ni", -1}};
    assert_holds_error("Ni", CODE_BAD_VALUE, document);

    // enclosing in list must result in same error
    document[key] = {{{"Ni", -1}}};
    assert_holds_error("Ni", CODE_BAD_VALUE, document);

    document[key] = {{"Ni", "1"}};
    assert_holds_error("Ni", CODE_TYPE_ERROR, document);

    // enclosing in list must result in same error
    document[key] = {{{"Ni", "1"}}};
    assert_holds_error("Ni", CODE_TYPE_ERROR, document);

    document[key] = {{"_Ni", "1"}};
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);

    // enclosing in list must result in same error
    document[key] = {{{"_Ni", "1"}}};
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);

    document[key] = {{"Ni", 3}, {"Fe", 2}};
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);

    // enclosing in list must result in same error
    document[key] = {{{"Ni", 3}, {"Fe", 2}}};
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);
  }

  TEST(test_parse_composition, error_which) {
    auto s = TEST_FCC_STRUCTURE<double>;
    json document{
          {"structure", {{"lattice", s.lattice}, {"coords", s.frac_coords}, {"species", s.species}}}};
    auto key = "composition";

    auto parse_composition = [](json const& doc) -> parse_result_t<std::vector<sublattice>> {
      auto structure = io::config::parse_structure_config<"structure", double>(doc);
      return io::config::parse_composition<"composition">(doc, get_result(structure).species);
    };

    auto assert_holds_error = make_assert_holds_error(parse_composition);
    assert_holds_error(key, CODE_NOT_FOUND, document);

    // test invalid index
    document[key] = {{"Ni", 2}, {"Co", 2}, {"sites", {-1, 2}}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE, document);
    // test invalid indices empty
    document[key] = {{"Ni", 2}, {"Co", 2}, {"sites", std::vector<int>{}}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE, document);

    // test too few sites
    document[key] = {{"Ni", 2}, {"Co", 2}, {"sites", {0, 2}}};
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);

    // test species not in structure
    document[key] = {{"Ni", 2}, {"Co", 2}, {"sites", "Fr"}};
    assert_holds_error("sites", CODE_OUT_OF_RANGE, document);

    // test too few sites - by symbol
    document[key] = {{"Ni", 2}, {"Co", 2}, {"sites", {"Si", "Mg"}}};
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);

    // test too many sites - by symbol
    document[key] = {{"Ni", 1}, {"_Co", 1}, {"sites", {"Si", "Mg"}}};
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);
  }
  TEST(test_parse_composition, error_multiple) {
    auto s = TEST_FCC_STRUCTURE<double>;
    json document{
              {"structure", {{"lattice", s.lattice}, {"coords", s.frac_coords}, {"species", s.species}}}};
    auto key = "composition";

    auto parse_composition = [](json const& doc) -> parse_result_t<std::vector<sublattice>> {
      auto structure = io::config::parse_structure_config<"structure", double>(doc);
      return io::config::parse_composition<"composition">(doc, get_result(structure).species);
    };

    auto assert_holds_error = make_assert_holds_error(parse_composition);
    assert_holds_error(key, CODE_NOT_FOUND, document);

    // test invalid index
    json sl1 = {{"Ni", 1}, {"Co", 1}, {"sites", {0, 1}}};
    json sl2 = {{"Ni", 1}, {"Co", 1}, {"sites", {1, 2}}};
    document[key] = {sl1, sl2};
    assert_holds_error("sites", CODE_BAD_VALUE, document);

    // overlap by species array
    document[key][1]["sites"] = {"Na", "Si"};
    assert_holds_error("sites", CODE_BAD_VALUE, document);

    // sublattice overlap by species definition
    document[key][1]["sites"] = "Mg";
    assert_holds_error("sites", CODE_BAD_VALUE, document);

    document[key][1].erase("sites");
    document[key][1]["Ni"] = 2;
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);
  }

  /*TEST(test_parse_structure, which) {
    auto s = TEST_FCC_STRUCTURE<double>;

    json base{{"lattice", s.lattice}, {"coords", s.frac_coords}, {"species", s.species}};


    base["which"] = std::vector<int>{};
    assert_holds_error<"which", CODE_OUT_OF_RANGE>(base);

    // atomic speices that is not contained in the structure
    base["which"] = "Fe";
    assert_holds_error<"which", CODE_TYPE_ERROR>(base);

    // atomic speices that is not contained in the structure
    base["which"] = {"Fe"};
    assert_holds_error<"which", CODE_OUT_OF_RANGE>(base);

    // index out of range
    base["which"] = std::array{1, 2, 3, 4};
    assert_holds_error<"which", CODE_OUT_OF_RANGE>(base);

    base["which"] = std::array{1, 2};
    auto r1 = io::parser::from_json<double>(base);
    ASSERT_TRUE(holds_result(r1));
    helpers::assert_structure_equal(get_result(r1).structure(), s.sliced(std::vector{1, 2}));

    base["which"] = std::array{"Si"};
    auto r2 = io::parser::from_json<double>(base);
    ASSERT_TRUE(holds_result(r2));
    helpers::assert_structure_equal(get_result(r2).structure(), s.sliced(std::vector{3}));
  }*/
}  // namespace sqsgen::testing