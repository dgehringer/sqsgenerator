//
// Created by Dominik Gehringer on 20.04.26.
//

#include "sqsgen/core/permutation.h"

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/numeric.h"

namespace sqsgen::core {
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <ranges::input_range R, class T> rank_t num_permutations_impl(R&& freqs) {
    T num_atoms{0};
    rank_t denominator{1};
    for (auto freq : freqs) {
      num_atoms += freq;
      denominator *= helpers::factorial<rank_t, T>(freq);
    }

    return helpers::factorial<rank_t, T>(num_atoms) / denominator;
  }

  rank_t num_permutations(counter<specie_t> const& hist) {
    return num_permutations_impl(views::values(hist));
  }

  rank_t num_permutations(configuration_t const& conf) {
    return num_permutations(helpers::count(conf));
  }

  rank_t rank_permutation(configuration_t const& configuration) {
    auto num_atoms{configuration.size()}, num_species{helpers::count(configuration).size()};
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

  void unrank_permutation(configuration_t& configuration, rank_t const& to_rank) {
    using namespace core::helpers;
    auto freqs = count(configuration);
    rank_t total_permutations{num_permutations(freqs)}, rank{to_rank};
    if (rank > total_permutations) {
      throw std::out_of_range("The rank is larger than the total number of permutations");
    }
    auto k{0};
    auto num_atoms{configuration.size()};
    auto num_species{freqs.size()};
    std::vector<std::size_t> hist;
    for (const auto& item : freqs) hist.push_back(item.second);

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

  bool next_permutation(configuration_t::iterator start, configuration_t::iterator end) {
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
