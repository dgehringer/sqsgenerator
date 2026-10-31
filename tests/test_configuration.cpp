//
// End-to-end tests for the configuration parser: drive the public API
// (configuration::read_input + configuration::parse_configuration) with JSON
// inputs, exactly as the CLI / WASM frontends do.
//
// Input documents live as individual files under tests/cases/ (one per
// scenario). SQSGEN_TEST_CASES_DIR is injected by CMake and points at that
// directory in the source tree, so new cases can be added by dropping a .json
// file in there -- no rebuild required.
//

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

#include "sqsgen/configuration.h"
#include "sqsgen/types.h"

#ifndef SQSGEN_TEST_CASES_DIR
#  define SQSGEN_TEST_CASES_DIR "cases"
#endif

namespace sqsgen::testing {

  namespace {
    // Reads the raw contents of tests/cases/<name>.
    std::string load_case(std::string_view name) {
      const auto path = std::filesystem::path{SQSGEN_TEST_CASES_DIR} / name;
      std::ifstream file{path};
      if (!file)
        throw std::runtime_error(std::format("could not open test case: {}", path.string()));
      std::stringstream buffer;
      buffer << file.rdbuf();
      return buffer.str();
    }

    // Reads + validates a case document, returning the parsed configuration variant.
    auto parse_case(std::string_view name) {
      const std::string json = load_case(name);
      auto input = configuration::read_input(json, io::wire_format::json);
      REQUIRE(input.has_value());
      return configuration::parse_configuration(input.value());
    }
  }  // namespace

  TEST_CASE("a well-formed configuration parses and validates", "[configuration]") {
    auto parsed = parse_case("valid_random.json");
    REQUIRE(parsed.has_value());
    // prec "double" + the default (interact) sublattice mode selects this alternative.
    CHECK(std::holds_alternative<configuration::configuration<double, sublattice_mode::interact>>(
        parsed.value()));
  }

  TEST_CASE("the prec field selects the single-precision alternative", "[configuration]") {
    auto parsed = parse_case("valid_single.json");
    REQUIRE(parsed.has_value());
    CHECK(std::holds_alternative<configuration::configuration<float, sublattice_mode::interact>>(
        parsed.value()));
  }

  TEST_CASE("systematic mode rejects an explicit iteration count", "[configuration]") {
    auto parsed = parse_case("systematic_with_iterations.json");
    REQUIRE_FALSE(parsed.has_value());
    CHECK(parsed.error().key == "iterations");
  }

  TEST_CASE("a composition that does not fill the lattice is rejected", "[configuration]") {
    auto parsed = parse_case("composition_mismatch.json");
    REQUIRE_FALSE(parsed.has_value());
    INFO("error key: " << parsed.error().key << ", msg: " << parsed.error().msg);
    CHECK_FALSE(parsed.error().key.empty());
  }

  TEST_CASE("malformed JSON fails at the read_input stage", "[configuration]") {
    auto input = configuration::read_input("{ this is : not json", io::wire_format::json);
    CHECK_FALSE(input.has_value());
  }

}  // namespace sqsgen::testing
