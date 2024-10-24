//
// Created by Dominik Gehringer on 19.09.24.
//

#ifndef SQSGEN_PERMUTATION_HPP
#define SQSGEN_PERMUTATION_HPP

#include <numeric>
#include <ranges>

#include "sqsgen/types.h"
#include "sqsgen/core/helpers.h"


namespace sqsgen::core {
  namespace ranges = std::ranges;

  template <ranges::range R>
    requires std::is_same_v<ranges::range_value_t<R>, specie_t>
  counter<specie_t> count_species(const R &r) {
    counter<specie_t> result{};
    for (auto specie : r) {
      if (result.contains(specie))
        ++result[specie];
      else
        result.emplace(specie, 1);
    }
    return result;
  }

  template <typename Out, typename In> Out factorial(In n) {
    if (n < 0) throw std::invalid_argument("n must be positive");
    if (n == 1) return 1;
    return helpers::fold_left(ranges::views::iota(In{1}, n + 1), Out{1}, std::multiplies{});
  }

  rank_t num_permutations(const configuration_t &conf);

  rank_t num_permutations(const counter<specie_t> &hist);

  template <ranges::input_range R, class T = ranges::range_value_t<R> >
  rank_t num_permutations(const R &freqs) {
    auto num_atoms = helpers::fold_left_first(freqs, std::plus{}).value_or(0);
    return factorial<rank_t, T>(num_atoms)
           / helpers::fold_left_first(freqs | ranges::views::transform(factorial<rank_t, T>),
                             std::multiplies{})
                 .value_or(1);
  }

  rank_t rank_permutation(const configuration_t &configuration);

  configuration_t unrank_permutation(const configuration_t &configuration, rank_t rank);

  configuration_t unrank_permutation(const configuration_t &configuration,
                                     const counter<specie_t> &hist, rank_t rank);

  bool next_permutation(configuration_t &configuration);
}  // namespace sqsgen::core

#endif  // SQSGEN_PERMUTATION_HPP
