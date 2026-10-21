//
// Created by Dominik Gehringer on 20.06.26.
//
#include "sqsgen/core/permutation.h"

#include <algorithm>
#include <ranges>

#include "sqsgen/core/helpers/count.h"
#include "sqsgen/core/helpers/fold.h"

namespace sqsgen::core {

  counter<specie_t> count_species(configuration_t const& c) { return helpers::count(c); }

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

  template <ranges::input_range R, class T = ranges::range_value_t<R>>
  rank_t num_permutations_impl(R&& freqs) {
    auto num_atoms = helpers::fold_left_first(freqs, std::plus{}).value_or(0);
    return factorial<rank_t, T>(num_atoms)
           / helpers::fold_left_first(freqs | ranges::views::transform(factorial<rank_t, T>),
                                      std::multiplies{})
                 .value_or(1);
  }

  static rank_t num_permutations(counter<specie_t> const& hist) {
    return num_permutations_impl(std::ranges::views::values(hist));
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

  inline void unrank_permutation(configuration_t& configuration, rank_t const& to_rank) {
    using namespace core::helpers;
    auto freqs = count(configuration);
    rank_t total_permutations{num_permutations(freqs)}, rank{to_rank};
    if (rank > total_permutations) {
      throw std::out_of_range("The rank is larger than the total number of permutations");
    }
    auto k{0};
    auto num_atoms{configuration.size()};
    auto num_species{freqs.size()};
    auto hist = std::ranges::views::values(freqs) | std::ranges::to<std::vector>();

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
