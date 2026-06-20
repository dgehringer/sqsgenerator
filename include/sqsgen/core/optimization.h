//
// Created by Dominik Gehringer on 18.03.25.
//

#ifndef SQSGEN_CORE_OPTIMIZATION_H
#define SQSGEN_CORE_OPTIMIZATION_H

#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::core::optimization {
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <class T> cube_t<T> scaled_pair_weights(cube_t<T> const& pair_weights,
                                                   shell_weights_t<T> const& weights,
                                                   usize_t num_species);

  template <class T> auto compute_shuffling_bounds(structure<T> const& structure,
                                                   std::vector<sublattice> const& composition);

  void count_bonds(cube_t<usize_t>& bonds, auto const& pairs, configuration_t const& species);

  template <class T> T compute_objective(cube_t<T>& sro, cube_t<usize_t> const& bonds,
                                         cube_t<T> const& prefactors, cube_t<T> const& pair_weights,
                                         cube_t<T> const& target, auto num_shells,
                                         auto num_species);
}  // namespace sqsgen::core::optimization

#endif  // SQSGEN_CORE_OPTIMIZATION_H
