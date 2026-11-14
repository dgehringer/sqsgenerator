//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_OPTIMIZATION_SRO_H
#define SQSGEN_OPTIMIZATION_SRO_H

#include "sqsgen/types.h"

namespace sqsgen::optimization {

  /**
   * @brief Per-cell coefficients of one linearized objective term.
   *
   * A term is evaluated as  s * |A + N * B|  over the canonical cells of a tensor
   * (see compute_pair_objective). The three tensors share the term's shape
   * (pairs: (shell, xi, eta); triplets: (class, xi, eta, zeta)):
   *   sro:    A = omega (target - 1),  B = omega f,  s = 1   (omega >= 0; lambda and 1/W folded in)
   *   energy: A = 0,                   B = |J|,      s = sign(J)
   * s = 0 marks a cell that does not contribute (omega = 0 or J = 0).
   */
  template <class Tensor> struct objective_terms {
    Tensor A;
    Tensor B;
    Tensor s;
  };

  /**
   * @brief Count the bonds of a configuration into a dense per-cell tensor. (Hot path.)
   *
   * @param counts   Output tensor (shell, species_i, species_j); reset to zero, then filled.
   * @param pairs    Precomputed pair list (see make_pair_list).
   * @param species  Species assigned to each site.
   */
  void count_pairs(pair_tensor_t<count_t>& counts, const core::pair_list_t& pairs,
                   const configuration_t& species);

  /**
   * @brief Evaluate the pair objective for a given bond count. (Hot path.)
   *
   *   O = sum_s sum_{xi <= eta} s(s, xi, eta) * | A(s, xi, eta) + N^s_{xi,eta} * B(s, xi, eta) |
   *
   * The bond count is read at the canonical cell xi <= eta, where the pair list
   * stored each bond exactly once:
   *   N^s_{xi,eta} = bonds(s, xi, eta) + bonds(s, eta, xi)   for xi < eta
   *   N^s_{xi,xi}  = bonds(s, xi, xi)
   *
   * @tparam T  Floating-point scalar type.
   * @param bonds   Bond counts per cell (see count_pairs).
   * @param terms   Precomputed coefficients A, B, s (see sro_terms / energy_terms).
   * @return The scalar objective value O; lower is closer to the target.
   */
  template <class T> T compute_pair_objective(const pair_tensor_t<count_t>& bonds,
                                              const objective_terms<pair_tensor_t<T>>& terms);

  /**
   * @brief Count triangles per class and species triple into a dense tensor. (Hot path.)
   *
   *   N^c(x1, x2, x3) = number of triangles of class c whose sites, in stored order,
   *                     carry the species (x1, x2, x3).
   *
   * Sites keep their canonical geometric order; no symmetrization happens here, as
   * interchangeable orderings are summed later in the objective.
   *
   * @param counts   Output tensor (class, x1, x2, x3); reset to zero, then filled.
   * @param triplet  Precomputed triangle list (see make_triplet_list).
   * @param species  Species assigned to each site.
   */
  void count_triplets(triplet_tensor_t<count_t>& counts, const core::triplet_list_t& triplet,
                      const configuration_t& species);
}  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/sro_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_SRO_H
