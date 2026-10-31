//
// Created by Dominik Gehringer on 19.09.24.
//

#ifndef SQSGEN_CORE_PERMUTATION_HPP
#define SQSGEN_CORE_PERMUTATION_HPP

#include "sqsgen/types.h"

namespace sqsgen::core {
  composition_t count_species(const configuration_t& c);
  rank_t num_permutations(const composition_t& hist);
  rank_t num_permutations(const configuration_t& conf);
  rank_t rank_permutation(const configuration_t& configuration);
  void unrank_permutation(configuration_t& configuration, const rank_t& to_rank);
  bool next_permutation(configuration_t::iterator start, configuration_t::iterator end);
}  // namespace sqsgen::core

#include "sqsgen/impl/core/permutation_impl.hpp"

#endif  // SQSGEN_CORE_PERMUTATION_HPP
