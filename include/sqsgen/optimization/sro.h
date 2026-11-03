//
// Created by Dominik Gehringer on 18.03.25.
//

#ifndef SQSGEN_OPTIMIZATION_H
#define SQSGEN_OPTIMIZATION_H

#include <utility>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  template <class T> struct pair_objective_coefficients {
    pair_tensor_t<T> a;
    pair_tensor_t<T> b;
  };

  template <class T>
  pair_list_t make_pair_list(const core::structure<T>& structure, const std::vector<T>& radii,
                             const std::map<shell_t, T>& weights, T atol, T rtol);

  template <class T> pair_tensor_t<T> scaled_pair_weights(const pair_tensor_t<T>& pair_weights,
                                                          const std::map<shell_t, T>& weights);
  template <class T>
  pair_objective_coefficients<T> pair_coefficients(const pair_tensor_t<T>& scaled_pair_weights,
                                                   const pair_tensor_t<T>& alpha_target,
                                                   const pair_tensor_t<T>& prefactors);

  template <class T> auto compute_shuffling_bounds(structure<T> const& structure,
                                                   std::vector<sublattice> const& composition);

  void count_pairs(pair_tensor_t<count_t>& counts, const pair_list_t& pairs,
                   const configuration_t& species);

  template <class T> T compute_pair_objective(const pair_tensor_t<count_t>& bonds,
                                              const pair_objective_coefficients<T>& coeffs);

  template <class T> std::pair<triplet_list_t, triplet_class_mapping_t> make_triplet_list(
      const core::structure<T>& structure, const std::vector<T>& radii,
      const std::map<shell_t, T>& weights, T atol, T rtol);

  void count_triplets(triplet_tensor_t<count_t>& counts, const triplet_list_t& triplet,
                      const configuration_t& species);
}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_H
