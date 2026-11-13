#ifndef SQSGEN_IMPL_OPTIMIZATION_PERMUTATION_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_PERMUTATION_IMPL_HPP

#include <algorithm>
#include <cassert>
#include <limits>
#include <numeric>
#include <random>
#include <utility>

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/rapidhash.h"
#include "sqsgen/optimization/permutation.h"

namespace sqsgen::optimization {

  // ---------------------------------------------------------------- layout

  template <std::ranges::input_range R>
  site_layout make_site_layout(std::size_t num_sites, R&& sublattice_sites) {
    // owner[site] = sublattice index; sites in no sublattice get "none" and sort last
    std::vector<std::size_t> owner(num_sites, std::numeric_limits<std::size_t>::max());
    std::vector<std::size_t> sizes;
    for (const auto& sites : sublattice_sites) {
      const std::size_t sl = sizes.size();
      std::size_t size = 0;
      for (const auto site : sites) {
        assert(static_cast<std::size_t>(site) < num_sites);
        assert(owner[site] == std::numeric_limits<std::size_t>::max());  // no overlap
        owner[site] = sl;
        ++size;
      }
      sizes.push_back(size);
    }

    site_layout layout;
    layout.order.resize(num_sites);
    std::iota(layout.order.begin(), layout.order.end(), site_t{0});
    // stable: input order is kept inside each sublattice, so the layout is deterministic
    std::ranges::stable_sort(layout.order, {}, [&](site_t s) { return owner[s]; });

    layout.ranges.reserve(sizes.size());
    std::size_t begin = 0;
    for (const std::size_t size : sizes) {
      layout.ranges.push_back({begin, begin + size});
      begin += size;
    }
    return layout;
  }

  inline std::span<specie_t> view(configuration_t& configuration, site_range r) {
    assert(r.begin <= r.end && r.end <= configuration.size());
    return std::span<specie_t>(configuration).subspan(r.begin, r.end - r.begin);
  }

  inline std::span<const specie_t> view(const configuration_t& configuration, site_range r) {
    assert(r.begin <= r.end && r.end <= configuration.size());
    return std::span<const specie_t>(configuration).subspan(r.begin, r.end - r.begin);
  }

  inline std::uint64_t next_u64(rng& g) {
    g.state += rapid_secret[0];
    return rapid_mix(g.state, g.state ^ rapid_secret[1]);
  }

  inline std::uint32_t bounded(rng& g, std::uint32_t n) {
    // upper 32 bits are the best ones; (x * n) / 2^32 maps [0, 2^32) onto [0, n)
    const auto x = static_cast<std::uint32_t>(next_u64(g) >> 32);
    return static_cast<std::uint32_t>((std::uint64_t{x} * n) >> 32);
  }

  constexpr std::uint64_t splitmix64(std::uint64_t x) {
    x += core::helpers::golden_ratio_u64;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
  }

  constexpr rng make_stream(std::uint64_t seed, std::uint64_t stream) {
    return rng{splitmix64(seed ^ splitmix64(stream))};
  }

  inline void seed_streams(std::span<rng> generators, std::span<const std::uint64_t> seeds,
                           std::uint64_t block) {
    assert(generators.size() == seeds.size());
    for (std::size_t r = 0; r < generators.size(); ++r)
      generators[r] = make_stream(seeds[r], block);
  }

  inline std::uint64_t make_random_seed() {
    std::random_device device;
    // random_device returns 32-bit values; combine two
    return (std::uint64_t{device()} << 32) | std::uint64_t{device()};
  }

  inline void shuffle(std::span<specie_t> range, rng& g) {
    for (std::size_t i = range.size(); i > 1; --i) {
      const std::size_t j = bounded(g, static_cast<std::uint32_t>(i));  // j in [0, i)
      std::swap(range[i - 1], range[j]);
    }
  }

  inline void shuffle(configuration_t& configuration, std::span<const site_range> ranges,
                      std::span<rng> generators) {
    assert(ranges.size() == generators.size());
    for (std::size_t r = 0; r < ranges.size(); ++r)
      shuffle(view(configuration, ranges[r]), generators[r]);
  }

  inline std::vector<std::size_t> species_histogram(std::span<const specie_t> range) {
    const std::size_t size = range.empty() ? 0 : std::size_t{std::ranges::max(range)} + 1;
    std::vector<std::size_t> hist(size, 0);
    for (const specie_t s : range) ++hist[s];
    return hist;
  }

  // incremental build keeps each division exact; see num_permutations in permutation.h.
  inline rank_t num_permutations(std::span<const std::size_t> hist) {
    rank_t total{1};
    std::size_t n = 0;
    for (const std::size_t h : hist)
      for (std::size_t k = 1; k <= h; ++k) {
        ++n;
        total = total * n / k;
      }
    return total;
  }

  inline rank_t num_permutations(std::span<const specie_t> range) {
    return num_permutations(species_histogram(range));
  }

  inline rank_t rank(std::span<const specie_t> range) {
    const std::size_t L = range.size();
    std::vector<std::size_t> hist(L == 0 ? 0 : std::size_t{std::ranges::max(range)} + 1, 0);
    rank_t result{0};
    rank_t suffix{1};
    for (std::size_t i = 0; i < L; ++i) {
      const specie_t x = range[L - 1 - i];
      ++hist[x];
      for (specie_t j = 0; j < x; ++j)
        if (hist[j] > 0) result += suffix * hist[j] / hist[x];  // product first: exact
      suffix = suffix * (i + 1) / hist[x];
    }
    return result;
  }

  // block-skipping over arrangements by leading species; see unrank in permutation.h.
  inline void unrank(std::span<specie_t> range, rank_t r) {
    auto hist = species_histogram(range);
    rank_t total = num_permutations(hist);
    assert(r >= 0 && r < total);  // ranks come from the scheduler, not from the user
    for (std::size_t p = 0; p < range.size(); ++p) {
      const std::size_t remaining = range.size() - p;
      for (std::size_t s = 0; s < hist.size(); ++s) {
        if (hist[s] == 0) continue;
        const rank_t block = total * hist[s] / remaining;  // exact
        if (r < block) {
          range[p] = static_cast<specie_t>(s);
          total = block;
          --hist[s];
          break;
        }
        r -= block;
      }
    }
  }

  inline bool next(std::span<specie_t> range) {
    return std::next_permutation(range.begin(), range.end());
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_PERMUTATION_IMPL_HPP
