#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "sqsgen/optimization/permutation.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using namespace sqsgen::core;

  namespace {
    const configuration_t empty{};
    const configuration_t conf_first_2{0, 1};
    const configuration_t conf_last_2{1, 0};

    const configuration_t conf_first_16{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};
    const configuration_t conf_last_16{1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};

    const configuration_t conf_last_24{2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
                                       1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
    const configuration_t conf_first_24{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
                                        1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2};

    const configuration_t conf_diff_20{0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                                       10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    const configuration_t conf_same_20{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    const std::vector<configuration_t> first_permutations{
        empty, conf_first_2, conf_first_16, conf_first_24, conf_same_20, conf_diff_20};

    const std::vector<configuration_t> last_permutations{empty, conf_last_2, conf_last_16,
                                                         conf_last_24, conf_same_20};

  }  // namespace

  rank_t compute_permutations(const std::vector<specie_t>& s) {
    return optimization::num_permutations(std::span<const specie_t>(s));
  }

  rank_t rank_permutation(const std::vector<specie_t>& s) {
    return optimization::rank(std::span<const specie_t>(s));
  }

  void unrank_permutation(std::vector<specie_t>& s, rank_t r) {
    optimization::unrank(std::span<specie_t>(s), r);
  }

  TEST_CASE("num_permutations of the empty configuration is 1", "[permutation]") {
    REQUIRE(compute_permutations(empty) == rank_t{1});
  }

  TEST_CASE("num_permutations counts distinct multiset permutations", "[permutation]") {
    REQUIRE(compute_permutations(conf_first_2) == rank_t{2});
    REQUIRE(compute_permutations(conf_last_2) == rank_t{2});
    REQUIRE(compute_permutations(conf_first_16) == rank_t{12870});
    REQUIRE(compute_permutations(conf_last_16) == rank_t{12870});
    REQUIRE(compute_permutations(conf_first_24) == rank_t{9465511770});
    REQUIRE(compute_permutations(conf_last_24) == rank_t{9465511770});
    REQUIRE(compute_permutations(conf_diff_20) == rank_t{"2432902008176640000"});
    REQUIRE(compute_permutations(conf_same_20) == rank_t{1});
  }

  TEST_CASE("the first permutation ranks to 0", "[permutation]") {
    for (const configuration_t& conf : first_permutations)
      REQUIRE(rank_permutation(conf) == rank_t{0});
  }

  TEST_CASE("the last permutation ranks to num_permutations - 1", "[permutation]") {
    for (const configuration_t& conf : last_permutations)
      REQUIRE(rank_permutation(conf) == compute_permutations(conf) - 1);
  }

  TEST_CASE("unranking rank 0 yields the first permutation", "[permutation]") {
    for (const configuration_t& conf : first_permutations) {
      configuration_t out(conf);
      unrank_permutation(out, rank_t{0});
      REQUIRE(conf == out);
      REQUIRE(rank_permutation(out) == rank_t{0});
    }
  }

  TEST_CASE("unranking the last rank yields the last permutation", "[permutation]") {
    for (const configuration_t& conf : last_permutations) {
      configuration_t out(conf);
      unrank_permutation(out, compute_permutations(conf) - 1);
      REQUIRE(conf == out);
      REQUIRE(rank_permutation(out) == compute_permutations(conf) - 1);
    }
  }

  TEST_CASE("rank and unrank are inverse across the whole orbit", "[permutation]") {
    configuration_t out(conf_first_16);
    const rank_t total = compute_permutations(conf_first_16);
    for (rank_t i = 0; i < total; ++i) {
      unrank_permutation(out, i);
      REQUIRE(rank_permutation(out) == i);
    }
  }

  TEST_CASE("next_permutation walks the orbit in rank order", "[permutation]") {
    configuration_t output(conf_first_16), helper(conf_first_16);
    const rank_t total = compute_permutations(conf_first_16);
    for (rank_t i = 0; i < total - 1; ++i) {
      unrank_permutation(helper, i);
      REQUIRE(output == helper);
      const bool result = next_permutation(output.begin(), output.end());
      REQUIRE(result);
      REQUIRE(rank_permutation(output) == i + 1);
    }
    REQUIRE(rank_permutation(output) == compute_permutations(output) - 1);
  }

}  // namespace sqsgen::testing
