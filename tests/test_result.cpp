//
// Tests for sqsgen::optimization::sqs_result (the header-only result record).
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "sqsgen/optimization/result.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using namespace sqsgen::optimization;
  using Catch::Matchers::WithinAbs;

  using interact_result = sqs_result<double, sublattice_mode::interact>;
  using split_result = sqs_result<double, sublattice_mode::split>;

  TEST_CASE("total_objective in interact mode returns the scalar objective", "[result]") {
    interact_result r{1.25, configuration_t{0, 1, 0, 1}, cube_t<double>{}};
    CHECK_THAT(r.total_objective(), WithinAbs(1.25, 1.0e-12));
  }

  TEST_CASE("total_objective in split mode sums the per-sublattice objectives", "[result]") {
    split_result r{std::vector<double>{0.5, 1.0, 2.25},
                   std::vector<configuration_t>{{0, 1}, {1, 0}, {0, 0}},
                   std::vector<cube_t<double>>{}};
    CHECK_THAT(r.total_objective(), WithinAbs(3.75, 1.0e-12));
  }

  TEST_CASE("equality compares only the configuration", "[result]") {
    interact_result a{1.0, configuration_t{0, 1, 2}, cube_t<double>{}};
    interact_result b{9.0, configuration_t{0, 1, 2}, cube_t<double>{}};  // different objective
    interact_result c{1.0, configuration_t{2, 1, 0}, cube_t<double>{}};

    CHECK(a == b);  // same configuration, different objective -> equal
    CHECK_FALSE(a == c);
  }

  TEST_CASE("sqs_result_hash agrees for equal configurations (interact)", "[result][hash]") {
    sqs_result_hash hash{};
    interact_result a{1.0, configuration_t{0, 1, 2, 3}, cube_t<double>{}};
    interact_result b{7.0, configuration_t{0, 1, 2, 3}, cube_t<double>{}};
    interact_result c{1.0, configuration_t{3, 2, 1, 0}, cube_t<double>{}};

    CHECK(hash(a) == hash(b));
    CHECK(hash(a) != hash(c));  // extremely unlikely to collide for a 4-element config
  }

  TEST_CASE("sqs_result_hash agrees for equal configurations (split)", "[result][hash]") {
    sqs_result_hash hash{};
    split_result a{std::vector<double>{1.0, 2.0}, std::vector<configuration_t>{{0, 1}, {1, 0}},
                   std::vector<cube_t<double>>{}};
    split_result b{std::vector<double>{5.0, 6.0}, std::vector<configuration_t>{{0, 1}, {1, 0}},
                   std::vector<cube_t<double>>{}};
    split_result c{std::vector<double>{1.0, 2.0}, std::vector<configuration_t>{{1, 0}, {0, 1}},
                   std::vector<cube_t<double>>{}};

    CHECK(hash(a) == hash(b));
    CHECK(hash(a) != hash(c));
  }

}  // namespace sqsgen::testing
