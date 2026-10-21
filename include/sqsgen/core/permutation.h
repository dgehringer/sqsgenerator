//
// Created by Dominik Gehringer on 19.09.24.
//

#ifndef SQSGEN_CORE_PERMUTATION_HPP
#define SQSGEN_CORE_PERMUTATION_HPP

#include "sqsgen/types.h"

namespace sqsgen::core {
  composition_t count_species(const configuration_t& c);
  static rank_t num_permutations(const composition_t& hist);
  static rank_t num_permutations(const configuration_t& conf);
  inline rank_t rank_permutation(const configuration_t& configuration);
  inline void unrank_permutation(configuration_t& configuration, const rank_t& to_rank);
  inline bool next_permutation(configuration_t::iterator start, configuration_t::iterator end);
}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_PERMUTATION_HPP
