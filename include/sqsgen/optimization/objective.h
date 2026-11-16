//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_OPTIMIZATION_OBJECTIVE_H
#define SQSGEN_OPTIMIZATION_OBJECTIVE_H

#include "sqsgen/types.h"

/**
 * @file
 * @brief Objective functions of one lattice.
 *
 * The counts are unsymmetrized: every pair / triangle increments exactly one cell.
 * The objective symmetrizes them: for each canonical cell it sums the counts of all cells
 * that describe the same cluster (Nbar), and evaluates one term per canonical cell.
 *
 *     sro:    term = s |A + Nbar B|
 *     energy: term = s B Nbar          (A = 0, B = |J|, s = sign J  =>  term = J Nbar)
 */

namespace sqsgen::optimization {

  using shell_triple = std::array<shell_t, 3>;

  template <class Tensor> struct objective_terms {
    Tensor A;
    Tensor B;
    Tensor s;
  };

  // O_pairs = sum over shells, xi <= eta of  s |A + Nbar B|
  template <class T> T compute_pair_sro_objective(const pair_tensor_t<count_t>& bonds,
                                                  const objective_terms<pair_tensor_t<T>>& terms);

  // O_triplets = sum over classes, canonical (xi, eta, zeta) of  s |A + Nbar B|
  template <class T>
  T compute_triplet_sro_objective(const triplet_tensor_t<count_t>& counts,
                                  const objective_terms<triplet_tensor_t<T>>& terms,
                                  const core::triplet_class_mapping_t& classes);

  // E_pairs = sum over shells, xi <= eta of  J Nbar = s B Nbar
  template <class T>
  T compute_pair_energy_objective(const pair_tensor_t<count_t>& bonds,
                                  const objective_terms<pair_tensor_t<T>>& terms);

  // E_triplets = sum over classes, canonical (xi, eta, zeta) of  J Nbar = s B Nbar
  template <class T>
  T compute_triplet_energy_objective(const triplet_tensor_t<count_t>& counts,
                                     const objective_terms<triplet_tensor_t<T>>& terms,
                                     const core::triplet_class_mapping_t& classes);

}  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/objective_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_OBJECTIVE_H
