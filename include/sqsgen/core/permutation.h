//
// Created by Dominik Gehringer on 19.09.24.
//

#ifndef SQSGEN_PERMUTATION_HPP
#define SQSGEN_PERMUTATION_HPP

#include "sqsgen/core/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::core {
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  inline counter<specie_t> count_species(configuration_t const& c) { return helpers::count(c); }

  template <ranges::input_range R, class T = ranges::range_value_t<R>>
  rank_t num_permutations_impl(R&& freqs) {
    auto num_atoms = helpers::fold_left_first(freqs, std::plus{}).value_or(0);
    return helpers::factorial<rank_t, T>(num_atoms)
           / helpers::fold_left_first(
                 freqs | ranges::views::transform(helpers::factorial<rank_t, T>), std::multiplies{})
                 .value_or(1);
  }

  static rank_t num_permutations(counter<specie_t> const& hist) {
    return num_permutations_impl(views::values(hist));
  }

  static rank_t num_permutations(configuration_t const& conf) {
    return num_permutations(count_species(conf));
  }

  inline rank_t rank_permutation(configuration_t const& configuration) {
    auto num_atoms{configuration.size()}, num_species{count_species(configuration).size()};
    rank_t rank{1}, suffix_permutations{1};
    std::vector hist(num_species, 0);

    for (auto i = 0; i < num_atoms; i++) {
      auto x = configuration[(num_atoms - 1) - i];
      hist[x]++;
      for (auto j = 0; j < num_species; j++) {
        if (j < x) {
          rank += suffix_permutations * hist[j] / hist[x];
        }
      }
      suffix_permutations = suffix_permutations * (i + 1) / hist[x];
    }
    return rank;
  }

  inline void unrank_permutation(configuration_t& configuration, rank_t const& to_rank,
                                 std::optional<bounds_t<usize_t>> bounds = std::nullopt) {
    using namespace core::helpers;
    auto b = bounds.value_or(bounds_t<usize_t>{0, configuration.size()});
    auto freqs = count(range(std::forward<decltype(b)>(b))
                       | views::transform([&](auto&& i) { return configuration[i]; }));
    rank_t total_permutations{num_permutations(freqs)}, rank{to_rank};
    if (rank > total_permutations) {
      throw std::out_of_range("The rank is larger than the total number of permutations");
    }
    auto k{0};
    auto num_atoms{std::get<1>(b) - std::get<0>(b)};
    auto num_species{freqs.size()};
    auto hist = helpers::as<std::vector>{}(views::values(freqs));

    for (auto i = 0; i < num_atoms; i++) {
      for (auto j = 0; j < num_species; j++) {
        rank_t suffix_count = total_permutations * hist[j] / (num_atoms - i);
        if (rank <= suffix_count) {
          configuration[k] = j;
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

  inline bool next_permutation(configuration_t& configuration) {
    auto num_atoms{configuration.size()};
    auto l{num_atoms - 1}, k{num_atoms - 2};

    while (configuration[k] >= configuration[k + 1]) {
      k -= 1;
      if (k < 0) {
        return false;
      }
    }
    while (configuration[k] >= configuration[l]) l -= 1;

    auto temporary = configuration[k];
    configuration[k] = configuration[l];
    configuration[l] = temporary;

    auto length = num_atoms - (k + 1);
    for (auto i = 0; i < length / 2; i++) {
      auto m = k + i + 1;
      auto n = num_atoms - i - 1;
      temporary = configuration[m];
      configuration[m] = configuration[n];
      configuration[n] = temporary;
    }
    return true;
  }
}  // namespace sqsgen::core

#endif  // SQSGEN_PERMUTATION_HPP
