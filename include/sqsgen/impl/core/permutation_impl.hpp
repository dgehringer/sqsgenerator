//
// Created by Dominik Gehringer on 20.06.26.
//

#ifndef SQSGEN_IMPL_CORE_PERMUTATION_IMPL_HPP
#define SQSGEN_IMPL_CORE_PERMUTATION_IMPL_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "sqsgen/core/helpers/count.h"
#include "sqsgen/core/helpers/fold.h"
#include "sqsgen/core/permutation.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  inline composition_t count_species(const configuration_t& c) { return helpers::count(c); }

  namespace detail {

    template <typename Out, typename In>
      requires std::is_convertible_v<In, Out>
    Out factorial(In n) {
      if (n < 0) throw std::invalid_argument("n must be positive");
      if (n == 1) return 1;
      Out result = 1;
      In i = 1;
      while (i <= n) {
        result *= i;
        ++i;
      }
      return result;
    }

    template <std::ranges::input_range R, class T = std::ranges::range_value_t<R>>
    rank_t num_permutations_impl(R&& freqs) {
      auto num_atoms = helpers::fold_left_first(freqs, std::plus{}).value_or(0);
      return factorial<rank_t, T>(num_atoms)
             / helpers::fold_left_first(freqs | std::ranges::views::transform(factorial<rank_t, T>),
                                        std::multiplies{})
                   .value_or(1);
    }

  }  // namespace detail

  inline rank_t num_permutations(const composition_t& hist) {
    return detail::num_permutations_impl(std::ranges::views::values(hist));
  }

  inline rank_t num_permutations(const configuration_t& conf) {
    return num_permutations(count_species(conf));
  }

  inline rank_t rank_permutation(const configuration_t& configuration) {
    auto num_atoms{configuration.size()}, num_species{count_species(configuration).size()};
    rank_t rank{1}, suffix_permutations{1};
    std::vector hist(num_species, 0);

    for (std::size_t i = 0; i < num_atoms; i++) {
      auto x = configuration[(num_atoms - 1) - i];
      hist[x]++;
      for (std::size_t j = 0; j < num_species; j++) {
        if (j < x) {
          rank += suffix_permutations * hist[j] / hist[x];
        }
      }
      suffix_permutations = suffix_permutations * (i + 1) / hist[x];
    }
    return rank;
  }

  inline void unrank_permutation(configuration_t& configuration, const rank_t& to_rank) {
    using namespace core::helpers;
    auto freqs = count(configuration);
    rank_t total_permutations{num_permutations(freqs)}, rank{to_rank};
    if (rank > total_permutations) {
      throw std::out_of_range("The rank is larger than the total number of permutations");
    }
    std::size_t k{0};
    auto num_atoms{configuration.size()};
    auto num_species{freqs.size()};
    auto hist = std::ranges::views::values(freqs) | std::ranges::to<std::vector>();

    for (std::size_t i = 0; i < num_atoms; i++) {
      for (std::size_t j = 0; j < num_species; j++) {
        rank_t suffix_count = total_permutations * hist[j] / (num_atoms - i);
        if (rank <= suffix_count) {
          configuration[k] = static_cast<specie_t>(j);
          total_permutations = suffix_count;
          hist[j]--;
          k++;
          if (hist[j] == 0) j++;
          break;
        }
        rank -= suffix_count;
      }
    }
  }

  inline bool next_permutation(configuration_t::iterator start, configuration_t::iterator end) {
    auto num_atoms{std::distance(start, end)};
    if (num_atoms < 2) return false;
    auto l{num_atoms - 1}, k{num_atoms - 2};

    while (start[k] >= start[k + 1]) {
      k -= 1;
      if (k < 0) {
        return false;
      }
    }
    while (start[k] >= start[l]) l -= 1;

    std::swap(start[k], start[l]);

    auto length = num_atoms - (k + 1);
    for (auto i = 0; i < length / 2; i++) {
      auto m = k + i + 1;
      auto n = num_atoms - i - 1;
      std::swap(start[m], start[n]);
    }
    return true;
  }

}  // namespace sqsgen::core

#endif  // SQSGEN_IMPL_CORE_PERMUTATION_IMPL_HPP
