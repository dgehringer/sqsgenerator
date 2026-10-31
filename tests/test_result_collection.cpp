//
// Tests for sqsgen::optimization::sqs_result_collection (header-only).
//

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <utility>

#include "sqsgen/optimization/result_collection.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using namespace sqsgen::optimization;

  using interact_result = sqs_result<double, sublattice_mode::interact>;
  using interact_collection = sqs_result_collection<double, sublattice_mode::interact>;

  namespace {
    interact_result make(double objective, configuration_t configuration) {
      return interact_result{objective, std::move(configuration), cube_t<double>{}};
    }
  }  // namespace

  TEST_CASE("insert deduplicates by (objective, configuration)", "[result_collection]") {
    interact_collection coll;
    CHECK(coll.num_results() == 0);

    CHECK(coll.insert(make(1.0, {0, 1})));  // new
    CHECK(coll.insert(make(1.0, {1, 0})));  // same objective, different configuration
    CHECK(coll.num_results() == 2);
    CHECK(coll.count_for_objective(1.0) == 2);

    CHECK_FALSE(coll.insert(make(1.0, {0, 1})));  // exact duplicate -> rejected
    CHECK(coll.num_results() == 2);

    CHECK(coll.insert(make(2.0, {0, 1})));  // new objective bucket
    CHECK(coll.num_results() == 3);
    CHECK(coll.count_for_objective(2.0) == 1);
    CHECK(coll.count_for_objective(42.0) == 0);  // absent objective
  }

  TEST_CASE("nth_best_objective returns objectives in ascending order", "[result_collection]") {
    interact_collection coll;
    coll.insert(make(3.0, {0, 1}));
    coll.insert(make(1.0, {0, 1}));
    coll.insert(make(2.0, {0, 1}));

    CHECK(coll.nth_best_objective(0) == 1.0);
    CHECK(coll.nth_best_objective(1) == 2.0);
    CHECK(coll.nth_best_objective(2) == 3.0);
    CHECK(std::isinf(coll.nth_best_objective(3)));  // out of range
  }

  TEST_CASE("snapshot yields buckets sorted by objective", "[result_collection]") {
    interact_collection coll;
    coll.insert(make(2.0, {0, 1}));
    coll.insert(make(1.0, {0, 1}));
    coll.insert(make(1.0, {1, 0}));

    auto snap = coll.snapshot();
    REQUIRE(snap.size() == 2);
    CHECK(snap[0].first == 1.0);
    CHECK(snap[0].second.size() == 2);
    CHECK(snap[1].first == 2.0);
    CHECK(snap[1].second.size() == 1);
  }

  TEST_CASE("merge folds another collection in, preserving dedup", "[result_collection]") {
    interact_collection a;
    a.insert(make(1.0, {0, 1}));

    interact_collection b;
    b.insert(make(1.0, {1, 0}));  // same objective, different configuration
    b.insert(make(1.0, {0, 1}));  // duplicate of a's entry
    b.insert(make(2.0, {0, 1}));

    a.merge(std::move(b));

    CHECK(a.count_for_objective(1.0) == 2);  // {0,1} and {1,0}; the duplicate is dropped
    CHECK(a.count_for_objective(2.0) == 1);
    CHECK(a.num_results() == 3);
  }

}  // namespace sqsgen::testing
