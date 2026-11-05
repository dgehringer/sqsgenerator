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

  // ---------------------------------------------------------------- random numbers

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

  rank_t num_permutations(std::span<const std::size_t> hist);
  rank_t num_permutations(std::span<const specie_t> range);

  rank_t rank(std::span<const specie_t> range);

  void unrank(std::span<specie_t> range, rank_t r);

  bool next(std::span<specie_t> range);

}  // namespace sqsgen::optimization

#include "sqsgen/impl/permutation/permutation_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_PERMUTATION_H
