//
// Created by Dominik Gehringer on 09.11.24.
//

#ifndef SQSGEN_CORE_SHUFFLE_H
#define SQSGEN_CORE_SHUFFLE_H

#include <random>
#include <thread>
#include <utility>

#include "sqsgen/types.h"

namespace sqsgen::core {

  class shuffler_base {
  protected:
    std::map<std::thread::id, std::uint64_t> thread_seeds_;
    std::mutex thread_seeds_mutex_;
    std::uint64_t seed_;
    std::uint64_t seed_global_;
    shuffler_base(std::optional<std::uint64_t> seed = std::nullopt)
        : seed_(seed.value_or(random_seed())), seed_global_(seed_) {};

    [[nodiscard]] std::uint64_t &seed();
    [[nodiscard]] std::uint64_t &threadsafe_seed();

  public:
    static std::uint64_t random_seed();
  };

  template <IterationMode IMode, SublatticeMode SMode> class shuffler : shuffler_base {};

  template <> class shuffler<ITERATION_MODE_RANDOM, SUBLATTICE_MODE_INTERACT> : shuffler_base {
  private:
    bounds_t<usize_t> bounds_;

  public:
    explicit shuffler(bounds_t<uint64_t> bounds, std::optional<std::uint64_t> seed = std::nullopt)
        : shuffler_base(seed), bounds_(bounds) {}

    void shuffle(configuration_t &configuration, bool thread_safe = true);
  };

  template <> class shuffler<ITERATION_MODE_RANDOM, SUBLATTICE_MODE_SPLIT> : shuffler_base {
  private:
    std::vector<bounds_t<usize_t>> bounds_;

  public:
    explicit shuffler(std::vector<bounds_t<usize_t>> bounds,
                      std::optional<std::uint64_t> seed = std::nullopt)
        : shuffler_base(seed), bounds_(std::move(bounds)) {}

    void shuffle(configuration_t &configuration, bool thread_safe = true);
  };

  template <> class shuffler<ITERATION_MODE_SYSTEMATIC, SUBLATTICE_MODE_INTERACT> : shuffler_base {
  private:
    bounds_t<usize_t> bounds_;

    explicit shuffler(bounds_t<usize_t> bounds) : bounds_(std::move(bounds)) {};

  public:
    void next_permutation(configuration_t &configuration);
    rank_t rank_permutation(configuration_t &configuration);
    void unrank_permutation(configuration_t &configuration, rank_t rank);
  };

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_SHUFFLE_H
