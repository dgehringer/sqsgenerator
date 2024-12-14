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
#include "sqsgen/io/config/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using json = nlohmann::json;
  using namespace sqsgen::io;
  using namespace sqsgen::io::config;

  template <class Fn> auto make_assert_holds_error(Fn&& fn) {
    return [&](std::string const& key, parse_error_code code, nlohmann::json const& a) {
      auto r = fn(a);
      ASSERT_TRUE(r.failed());
      parse_error error = r.error();
      ASSERT_EQ(key, error.key) << error.msg;
      ASSERT_EQ(code, error.code) << error.msg;
    };
  }

  template <class T> const static auto TEST_FCC_STRUCTURE = core::structure<T>{
      lattice_t<T>{{1, 0, 0}, {0, 2, 0}, {0, 0, 3}},
      coords_t<T>{{0.0, 0.0, 0.0}, {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0}},
      std::vector<specie_t>{11, 12, 13, 14}};

  TEST(test_parse_structure, empty) {
    // ASSERT_TRUE(io::config::parse_structure_config<"composition", double>({}).failed());
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
    auto rdict = io::config::parse_structure_config<"structure", double>(py::object(dict));

    helpers::assert_structure_equal(s, rjson.result().structure());
    helpers::assert_structure_equal(rdict.result().structure(), rjson.result().structure());

    json["structure"]["species"] = std::vector<std::string>{"Na", "Mg", "Al", "Si"};
    dict["structure"]["species"] = std::vector<std::string>{"Na", "Mg", "Al", "Si"};
    rjson = io::config::parse_structure_config<"structure", double>(json);
    rdict = io::config::parse_structure_config<"structure", double>(py::object(dict));
    ASSERT_TRUE(rjson.ok());
    ASSERT_TRUE(rdict.ok());
    helpers::assert_structure_equal(s, rjson.result().structure());
    helpers::assert_structure_equal(rdict.result().structure(), rjson.result().structure());
  }

  TEST(test_parse_structure, required_fields_errors) {
    auto s = TEST_FCC_STRUCTURE<double>;
    json document = {{"structure",
                      {
                          {"lattice", s.lattice},
                          {"coords", s.frac_coords},
                          {"species", 4},
                      }}};

    auto assert_holds_error = [&](std::string const& key, parse_error_code code) {
      auto r = io::config::parse_structure_config<"structure", double>(document);
      ASSERT_TRUE(r.failed());
      parse_error error = r.error();
      ASSERT_EQ(key, error.key) << error.msg;
      ASSERT_EQ(code, error.code) << error.msg;
    };

    document["structure"].erase("species");
    assert_holds_error("species", CODE_NOT_FOUND);
    document["structure"]["species"] = {1, 2, 3};
    assert_holds_error("species", CODE_OUT_OF_RANGE);

    // test invalid atomic speices
    document["structure"]["species"] = {1, 2, 3, -5};
    assert_holds_error("species", CODE_OUT_OF_RANGE);

    document["structure"]["species"] = {"Al", "Mg", "Si", "??"};
    // test invalid atomic species
    assert_holds_error("species", CODE_OUT_OF_RANGE);

    document["structure"]["species"] = {"Al", "Mg", "Si", "Ge"};
    document["structure"]["coords"] = std::vector{1, 2, 3, 4};
    // test wrong shape for coords
    assert_holds_error("coords", CODE_TYPE_ERROR);

    // test wrong shape for lattice
    document["structure"]["lattice"] = s.frac_coords;
    document["structure"]["coords"] = s.lattice;
    assert_holds_error("lattice", CODE_OUT_OF_RANGE);
  }

  TEST(test_parse_composition, error_species) {
    auto s = TEST_FCC_STRUCTURE<double>;
    json document{
        {"structure", {{"lattice", s.lattice}, {"coords", s.frac_coords}, {"species", s.species}}}};
    auto key = "composition";

    auto parse_composition = [](json const& doc) -> parse_result<std::vector<sublattice>> {
      auto structure = io::config::parse_structure_config<"structure", double>(doc);
      return io::config::parse_composition<"composition">(doc, structure.result().species);
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

    auto parse_composition = [](json const& doc) -> parse_result<std::vector<sublattice>> {
      auto structure = io::config::parse_structure_config<"structure", double>(doc);
      return io::config::parse_composition<"composition">(doc, structure.result().species);
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

    auto parse_composition = [](json const& doc) -> parse_result<std::vector<sublattice>> {
      auto structure = io::config::parse_structure_config<"structure", double>(doc);
      return io::config::parse_composition<"composition">(doc, structure.result().species);
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

    // sublattice overlap by species definition
    /*document[key][1]["sites"] = {2};
    document[key][1]["Ni"] = 0;
    assert_holds_error("sites", CODE_BAD_VALUE, document);*/

    // create a sublatte with three Ni:2, Co: 1 atoms and have only two sites lift
    document[key][1].erase("sites");
    document[key][1]["Ni"] = 2;
    assert_holds_error(key, CODE_OUT_OF_RANGE, document);
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