//
// Created by Dominik Gehringer on 20.11.24.
//

#include <gtest/gtest.h>

#include "helpers.h"
#include "io/parser.h"
#include "nlohmann/json.hpp"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/parser.h"
#include "sqsgen/io/parser/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using json = nlohmann::json;
  using namespace sqsgen::io::parser;

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

  template <core::helpers::string_literal key, parse_error_code code, class ... Args>
  void assert_holds_error(nlohmann::json const& a) {
    auto r = io::parser::from_json<double>(a);
    ASSERT_TRUE(holds_error(r));
    parse_error error = std::get<parse_error>(r);
    ASSERT_EQ(key.data, error.key);
    ASSERT_EQ(code, error.code);
  }

  TEST(test_parse_structure, required_fields_errors) {
    auto s = TEST_FCC_STRUCTURE<double>;
    auto r1 = io::parser::from_json<double>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", 4},
    });
    assert_holds_error<"species", CODE_OUT_OF_RANGE>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", {1, 2, 3}},
    });
    assert_holds_error<"species", CODE_OUT_OF_RANGE>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", {1, 2, 3, -5}},
    });
    assert_holds_error<"species", CODE_OUT_OF_RANGE>({
        {"lattice", s.lattice},
        {"coords", s.frac_coords},
        {"species", {"Al", "Mg", "Si", "??"}},
    });
  }

}  // namespace sqsgen::testing