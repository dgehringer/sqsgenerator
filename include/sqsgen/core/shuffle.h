//
// Created by Dominik Gehringer on 09.11.24.
//

#ifndef SQSGEN_CORE_SHUFFLE_H
#define SQSGEN_CORE_SHUFFLE_H

#include <random>

#include "sqsgen/core/helpers/rapidhash.h"
#include "sqsgen/core/permutation.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  inline std::uint64_t make_random_seed() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
  }

  static uint64_t rapidrand(uint64_t &seed) {
    seed += rapid_secret[0];
    return rapid_mix(seed, seed ^ rapid_secret[1]);
  }

  static usize_t random_bounded(usize_t range, uint64_t &seed) {
    uint32_t x = rapidrand(seed);
    uint64_t m = static_cast<uint64_t>(x) * static_cast<uint64_t>(range);
    return m >> 32;
  }

  class shuffler {
  public:
    explicit shuffler(std::vector<bounds_t<usize_t>> bounds,
                      std::optional<std::uint64_t> seed = std::nullopt)
        : _seed(seed.value_or(make_random_seed())), _bounds(std::move(bounds)) {}
    template <IterationMode Mode> void shuffle(configuration_t &configuration) {
      if constexpr (Mode == ITERATION_MODE_RANDOM) {
        for (auto &bound : _bounds) {
          auto [lower_bound, upper_bound] = bound;
          assert(upper_bound > lower_bound);
          auto window_size = upper_bound - lower_bound;
          for (usize_t i = window_size; i > 1; i--) {
            usize_t p = random_bounded(i, _seed);  // number in [0,i)
            std::swap(configuration[lower_bound + i - 1],
                      configuration[p + lower_bound]);  // swap the values at i-1 and p
          }
        }
      } else if constexpr (Mode == ITERATION_MODE_SYSTEMATIC) {
        assert(_bounds.size() == 1);
        auto [lower_bound, upper_bound] = _bounds.front();
        auto result = next_permutation(configuration.begin() + lower_bound,
                                       configuration.begin() + upper_bound);
        /*std::next_permutation(configuration.begin() + lower_bound,
                              configuration.begin() + upper_bound);*/
      }
    }

    rank_t rank_permutation(configuration_t &configuration) {
      using namespace core::helpers;

      return core::rank_permutation(
          as<std::vector>{}(range(std::forward<bounds_t<usize_t>>(_bounds.front()))
                            | views::transform([&](auto &&i) { return configuration[i]; })));
    }

    template <IterationMode Mode>
    void unrank_permutation(configuration_t &configuration, rank_t rank) {
      static_assert(Mode == ITERATION_MODE_SYSTEMATIC);
      assert(_bounds.size() == 1);
      if (_bounds.empty() || _bounds.front() == bounds_t<usize_t>{0, configuration.size()})
        core::unrank_permutation(configuration, rank);
      else {
        auto [start, end] = _bounds.front();
        auto irange = helpers::range(std::forward<bounds_t<usize_t>>(_bounds.front()));
        auto [species_map, species_rmap] = helpers::make_index_mapping<specie_t>(
            irange | views::transform([&](auto &&i) { return configuration[i]; }));
        configuration_t conf{helpers::as<std::vector>{}(
            irange | views::transform([&](auto &&i) { return species_map.at(configuration[i]); }))};
        core::unrank_permutation(conf, rank);
        ranges::for_each(helpers::range(conf.size()), [&, start](auto &&i) {
          configuration[start + i] = species_rmap.at(conf[i]);
        });
        return;
      }
    }

  private:
    std::uint64_t _seed;
    std::vector<bounds_t<usize_t>> _bounds;
  };

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_SHUFFLE_H
