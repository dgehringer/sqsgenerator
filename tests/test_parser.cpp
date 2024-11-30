//
// Created by Dominik Gehringer on 20.11.24.
//

#include <gtest/gtest.h>

#include "helpers.h"
#include "io/parser.h"
#include "nlohmann/json.hpp"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/parser.h"
#include "sqsgen/io/parser/structure_config.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using json = nlohmann::json;
  using namespace sqsgen::io::parser;

  template <core::helpers::string_literal key, parse_error_code code, class... Args>
  void assert_holds_error(nlohmann::json const& a) {
    auto r = io::parser::from_json<double>(a);
    ASSERT_TRUE(holds_error(r));
    parse_error error = std::get<parse_error>(r);
    ASSERT_EQ(key.data, error.key);
    ASSERT_EQ(code, error.code) << error.msg;
  }

  template <class T> const static auto TEST_FCC_STRUCTURE = core::structure<T>{
      lattice_t<T>{{1, 0, 0}, {0, 2, 0}, {0, 0, 3}},
      coords_t<T>{{0.0, 0.0, 0.0}, {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0}},
      std::vector<specie_t>{11, 12, 13, 14}};

  TEST(test_parse_structure, empty) { ASSERT_TRUE(holds_error(io::parser::from_json<double>({}))); }

  TEST(test_parse_structure, required_fields_success) {
    auto s = TEST_FCC_STRUCTURE<double>;
    auto r1 = io::parser::from_json<double>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", s.species},
    });
    ASSERT_TRUE(holds_result(r1));
    helpers::assert_structure_equal(s, get_result(r1).structure());

    auto r2 = io::parser::from_json<double>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", std::vector<std::string>{"Na", "Mg", "Al", "Si"}},
    });
    ASSERT_TRUE(holds_result(r2));
    helpers::assert_structure_equal(s, get_result(r2).structure());
  }

  TEST(test_parse_structure, required_fields_errors) {
    auto s = TEST_FCC_STRUCTURE<double>;
    auto r1 = io::parser::from_json<double>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", 4},
    });
    assert_holds_error<"species", CODE_NOT_FOUND>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
    });
    assert_holds_error<"species", CODE_OUT_OF_RANGE>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", {1, 2, 3}},
    });

    // test invalid atomic speices
    assert_holds_error<"species", CODE_OUT_OF_RANGE>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", {1, 2, 3, -5}},
    });

    // test invalid atomic species
    assert_holds_error<"species", CODE_OUT_OF_RANGE>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", {"Al", "Mg", "Si", "??"}},
    });

    // test wrong shape for coords
    assert_holds_error<"coords", CODE_TYPE_ERROR>({
        {"lattice", s.lattice},
        {"coords", std::vector{1, 2, 3, 4}},
        {"species", {1, 2, 3}},
    });

    // test wrong shape for lattice
    assert_holds_error<"lattice", CODE_OUT_OF_RANGE>({
        {"lattice", s.frac_coords},
        {"coords", s.lattice},
        {"species", {1, 2, 3}},
    });
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