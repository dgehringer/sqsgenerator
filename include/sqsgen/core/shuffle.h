//
// Created by Dominik Gehringer on 09.11.24.
//

#ifndef SQSGEN_CORE_SHUFFLE_H
#define SQSGEN_CORE_SHUFFLE_H

#include <random>

#include "rapidhash.h"
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
    explicit shuffler(std::optional<std::uint64_t> seed = std::nullopt)
        : seed(seed.value_or(make_random_seed())) {}

    template<class T>
    void shuffle(std::vector<T> &configuration) {
      for (auto i = static_cast<uint32_t>(configuration.size()); i > 1; i--) {
        uint32_t p = random_bounded(i, seed);               // number in [0,i)
        std::swap(configuration[i - 1], configuration[p]);  // swap the values at i-1 and p
      }
    }

    void shuffle(configuration_t &configuration, std::vector<bounds_t<usize_t>> const& bounds) {
      for (auto &bound : bounds) {
        auto [lower_bound, upper_bound] = bound;
        assert(upper_bound > lower_bound);
        auto window_size = upper_bound - lower_bound;
        for (usize_t i = window_size; i > 1; i--) {
          usize_t p = random_bounded(i, seed);  // number in [0,i)
          std::swap(configuration[lower_bound + i - 1],
                    configuration[p + lower_bound]);  // swap the values at i-1 and p
        }
      }
    }

  private:
    std::uint64_t seed;
  };

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_SHUFFLE_H
