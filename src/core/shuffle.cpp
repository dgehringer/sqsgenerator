//
// Created by Dominik Gehringer on 20.04.26.
//
#include "sqsgen/core/shuffle.h"

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/rapidhash.h"
#include "sqsgen/core/permutation.h"

namespace sqsgen::core {

  namespace detail {
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

    static void knuth_fisher_yates(configuration_t &configuration, usize_t lower_bound,
                                   usize_t upper_bound, std::uint64_t &seed) {
      assert(upper_bound > lower_bound);
      auto window_size = upper_bound - lower_bound;
      for (usize_t i = window_size; i > 1; i--) {
        usize_t p = random_bounded(i, seed);  // number in [0,i)
        std::swap(configuration[lower_bound + i - 1],
                  configuration[p + lower_bound]);  // swap the values at i-1 and p
      }
    }

    configuration_t in_bounds(const configuration_t &configuration, bounds_t<usize_t> bounds) {
      auto [lower_bound, upper_bound] = bounds;
      configuration_t in_bounds;
      in_bounds.reserve(upper_bound - lower_bound);
      for (auto i = lower_bound; i < upper_bound; i++) in_bounds.push_back(configuration[i]);
      return in_bounds;
    }

  }  // namespace detail
     // namespace sqsgen::core

  std::uint64_t shuffler_base::random_seed() { return detail::make_random_seed(); }

  std::uint64_t &shuffler_base::seed() { return seed_global_; }

  std::uint64_t &shuffler_base::threadsafe_seed() {
    std::unique_lock lock(thread_seeds_mutex_);
    auto this_thread_id = std::this_thread::get_id();
    if (!thread_seeds_.contains(this_thread_id)) {
      thread_seeds_[this_thread_id] = seed_;
    }
    return thread_seeds_[this_thread_id];
  }

  void shuffler<ITERATION_MODE_RANDOM, SUBLATTICE_MODE_INTERACT>::shuffle(
      configuration_t &configuration, bool thread_safe) {
    std::uint64_t &seed = thread_safe ? threadsafe_seed() : this->seed();
    auto [lower_bound, upper_bound] = bounds_;
    detail::knuth_fisher_yates(configuration, lower_bound, upper_bound, seed);
  }

  void shuffler<ITERATION_MODE_RANDOM, SUBLATTICE_MODE_SPLIT>::shuffle(
      configuration_t &configuration, bool thread_safe) {
    std::uint64_t &seed = thread_safe ? threadsafe_seed() : this->seed();
    for (auto &bound : bounds_) {
      auto [lower_bound, upper_bound] = bound;
      detail::knuth_fisher_yates(configuration, lower_bound, upper_bound, seed);
    }
  }

  void shuffler<ITERATION_MODE_SYSTEMATIC, SUBLATTICE_MODE_INTERACT>::next_permutation(
      configuration_t &configuration) {
    auto [lower_bound, upper_bound] = bounds_;
    core::next_permutation(configuration.begin() + lower_bound,
                           configuration.begin() + upper_bound);
  }

  rank_t shuffler<ITERATION_MODE_SYSTEMATIC, SUBLATTICE_MODE_INTERACT>::rank_permutation(
      configuration_t &configuration) {
    return core::rank_permutation(detail::in_bounds(configuration, bounds_));
  }

  void shuffler<ITERATION_MODE_SYSTEMATIC, SUBLATTICE_MODE_INTERACT>::unrank_permutation(
      configuration_t &configuration, rank_t rank) {
    if (bounds_ == bounds_t<usize_t>{0, configuration.size()})
      core::unrank_permutation(configuration, rank);
    else {
      auto in_bounds = detail::in_bounds(configuration, bounds_);
      auto [species_map, species_rmap] = helpers::make_index_mapping<specie_t>(in_bounds);

      configuration_t packed;
      packed.reserve(in_bounds.size());
      std::transform(in_bounds.begin(), in_bounds.end(), std::back_inserter(packed),
                     [&](auto &&specie) { return species_map.at(specie); });

      core::unrank_permutation(packed, rank);
      configuration_t unpacked(configuration);
      auto [start, end] = bounds_;
      for (auto i = start; packed.size() < end; i++)
        unpacked[start + i] = species_rmap.at(packed[i]);
    }
  }
}  // namespace sqsgen::core
