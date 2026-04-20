//
// Created by Dominik Gehringer on 19.09.24.
//

#ifndef SQSGEN_CORE_PERMUTATION_HPP
#define SQSGEN_CORE_PERMUTATION_HPP

#include "sqsgen/types.h"

namespace sqsgen::core {
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <ranges::input_range R, class T = ranges::range_value_t<R>>
  rank_t num_permutations_impl(R&& freqs);

  rank_t num_permutations(counter<specie_t> const& hist);

  rank_t num_permutations(configuration_t const& conf);

  rank_t rank_permutation(configuration_t const& configuration);

  void unrank_permutation(configuration_t& configuration, rank_t const& to_rank);
  bool next_permutation(configuration_t::iterator start, configuration_t::iterator end);

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_PERMUTATION_HPP
