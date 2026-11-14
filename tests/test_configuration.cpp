//
// End-to-end tests for the configuration parser: drive the public API
// (configuration::read_input + configuration::parse_configuration) exactly as
// the CLI / WASM frontends do.
//
// Input documents live as individual files under tests/assets/ (one per
// scenario), named "case{num}.configuration.json". They are copied next to the
// test binary at build time (see tests/CMakeLists.txt), so new cases can be
// added by dropping a file in there -- no code change required.
//

#include <algorithm>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <glaze/glaze.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "sqsgen/configuration.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {

  namespace {
    // Reads the raw contents of tests/assets/<name>.
    std::string load_asset(std::string_view name) {
      const auto path = std::filesystem::current_path() / "assets" / name;
      std::ifstream file{path};
      if (!file) throw std::runtime_error(std::format("could not open asset: {}", path.string()));
      std::stringstream buffer;
      buffer << file.rdbuf();
      return buffer.str();
    }

    // Every "case{num}.configuration.json" under tests/assets/, sorted by name.
    std::vector<std::string> configuration_cases() {
      std::vector<std::string> names;
      const auto dir = std::filesystem::current_path() / "assets";
      for (const auto& entry : std::filesystem::directory_iterator{dir}) {
        const auto name = entry.path().filename().string();
        if (entry.is_regular_file() && name.ends_with(".configuration.json")) names.push_back(name);
      }
      std::ranges::sort(names);
      return names;
    }

    // Recovers the precision of a configuration alternative, to select the
    // matching cases per template instantiation.
    template <class> struct precision_of;
    template <class T, sublattice_mode Mode>
    struct precision_of<configuration::configuration<T, Mode>> {
      using type = T;
    };

    // Configurations embed Eigen matrices (no usable operator==), so compare
    // their canonical DTO serialization, which is deterministic for identical
    // values -- bit-equal fields serialize to the same JSON text.
    template <class T, sublattice_mode Mode>
    std::string serialize(const configuration::configuration<T, Mode>& config) {
      const auto written = glz::write_json(config.to_input());
      REQUIRE(written.has_value());
      return written.value();
    }
  }  // namespace

  // Round trip: read -> configuration_input (1) -> parse -> configuration (c)
  //                  -> to_input -> configuration_input (2) -> parse -> (c')
  // then assert c == c'. The second parse leaves c and c' in the same domain
  // space, so to_input is exercised as an exact inverse of parse.
  TEMPLATE_TEST_CASE("a configuration round-trips through to_input", "[configuration]", float,
                     double) {
    using T = TestType;

    for (const auto& name : configuration_cases()) {
      INFO("case: " << name);
      const std::string document = load_asset(name);

      auto input1 = configuration::read_input(document, io::wire_format::json);
      REQUIRE(input1.has_value());

      auto c = configuration::parse_configuration(input1.value());
      REQUIRE(c.has_value());

      std::visit(
          [&](const auto& config) {
            using config_t = std::decay_t<decltype(config)>;
            if constexpr (std::is_same_v<T, typename precision_of<config_t>::type>) {
              auto input2 = config.to_input();
              auto cc = configuration::parse_configuration(
                  configuration::configuration_input_variant_t{input2});
              REQUIRE(cc.has_value());
              CHECK(serialize(config) == serialize(std::get<config_t>(cc.value())));
            }
          },
          c.value());
    }
  }

  TEST_CASE("malformed JSON fails at the read_input stage", "[configuration]") {
    auto input = configuration::read_input("{ this is : not json", io::wire_format::json);
    CHECK_FALSE(input.has_value());
  }

}  // namespace sqsgen::testing
