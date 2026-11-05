#ifndef SQSGEN_OPTIMIZATION_PERMUTATION_H
#define SQSGEN_OPTIMIZATION_PERMUTATION_H

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

#include "sqsgen/types.h"

namespace sqsgen::optimization {

  // half-open range [begin, end) of positions in the (reordered) configuration
  struct site_range {
    std::size_t begin;
    std::size_t end;
  };

  struct site_layout {
    std::vector<site_t> order;
    std::vector<site_range> ranges;
  };

  template <std::ranges::input_range R>
  site_layout make_site_layout(std::size_t num_sites, R&& sublattice_sites);

  std::span<specie_t> view(configuration_t& configuration, site_range r);
  std::span<const specie_t> view(const configuration_t& configuration, site_range r);

  struct rng {
    std::uint64_t state;
  };

  std::uint64_t next_u64(rng& g);

  std::uint32_t bounded(rng& g, std::uint32_t n);

  constexpr std::uint64_t splitmix64(std::uint64_t x);

  constexpr rng make_stream(std::uint64_t seed, std::uint64_t stream);

  void seed_streams(std::span<rng> generators, std::span<const std::uint64_t> seeds,
                    std::uint64_t block);

  std::uint64_t make_random_seed();

  void shuffle(std::span<specie_t> range, rng& g);

  void shuffle(configuration_t& configuration, std::span<const site_range> ranges,
               std::span<rng> generators);

  std::vector<std::size_t> species_histogram(std::span<const specie_t> range);

  /**
   * @brief Number of distinct permutations of a multiset: L! / prod_s h_s!.
   *
   * Built incrementally (adding the k-th copy of a species at length n multiplies by n/k);
   * every intermediate value is a multinomial, so each division is exact.
   *
   * @param hist  Per-species counts h_s; L is their sum.
   * @return The multinomial coefficient as a rank_t.
   */
  rank_t num_permutations(std::span<const std::size_t> hist);
  rank_t num_permutations(std::span<const specie_t> range);

  /**
   * @brief Lexicographic rank of a multiset permutation.
   *
   * rank = sum over positions p of the permutations of the suffix starting at p that begin
   * with a species smaller than range[p], using perm(S with j in front) = perm(S) * h_j / |S|.
   *
   * @param range  The permutation, one species per position.
   * @return The 0-based rank among all distinct permutations of the same multiset.
   */
  rank_t rank(std::span<const specie_t> range);

  /**
   * @brief Inverse of rank: overwrite @p range with the permutation of the given rank.
   *
   * At each position the arrangements starting with a species s form a block of size
   * total * h_s / remaining; blocks are skipped until the rank falls inside one.
   *
   * @param range  In/out: holds the multiset on entry, the ranked permutation on return.
   * @param r      Rank in [0, num_permutations(range)); supplied by the scheduler, not the user.
   */
  void unrank(std::span<specie_t> range, rank_t r);

  bool next(std::span<specie_t> range);

}  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/permutation_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_PERMUTATION_H
