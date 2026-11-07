//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_OPTIMIZATION_SRO_H
#define SQSGEN_OPTIMIZATION_SRO_H

#include <utility>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  /**
   * @brief Precomputed per-cell coefficients for the linearized pair objective.
   *
   * The objective is evaluated as the sum of |a + N * b| over shells and canonical
   * species pairs (see compute_pair_objective). Both tensors are indexed
   * (shell, species_i, species_j).
   */
  template <class T> struct pair_objective_coefficients {
    pair_tensor_t<T> a;  ///< Constant term per cell: scaled_pair_weights * (alpha_target - 1).
    pair_tensor_t<T> b;  ///< Count coefficient per cell: scaled_pair_weights * prefactors.
  };

  /**
   * @brief Scale each shell slice of a pair-weight tensor by its shell weight.
   *
   * @tparam T  Floating-point scalar type.
   * @param pair_weights  Per-cell pair weights, indexed (shell, species_i, species_j).
   * @param weights       Shell -> weight map; slice s is multiplied by weights[s].
   * @return Copy of @p pair_weights with every selected shell slice scaled.
   */
  template <class T> pair_tensor_t<T> scaled_pair_weights(const pair_tensor_t<T>& pair_weights,
                                                          const std::map<shell_t, T>& weights);

  /**
   * @brief Precompute the constant (a) and count (b) coefficients of the pair objective.
   *
   *   a = scaled_pair_weights * (alpha_target - 1)
   *   b = scaled_pair_weights * prefactors
   *
   * @tparam T  Floating-point scalar type.
   * @param scaled_pair_weights  Shell-weighted pair weights (see scaled_pair_weights).
   * @param alpha_target         Target SRO parameters per cell.
   * @param prefactors           Normalization prefactors per cell.
   * @return Coefficients consumed by compute_pair_objective.
   */
  template <class T>
  pair_objective_coefficients<T> pair_coefficients(const pair_tensor_t<T>& scaled_pair_weights,
                                                   const pair_tensor_t<T>& alpha_target,
                                                   const pair_tensor_t<T>& prefactors);

  /**
   * @brief Group a structure's atoms by sublattice into contiguous index blocks.
   *
   * Reorders the sites so that every sublattice of @p composition occupies one
   * contiguous half-open index range, which lets the shuffler permute each
   * sublattice independently.
   *
   * @tparam T  Floating-point scalar type.
   * @param structure    Structure to reorder.
   * @param composition  Sublattices; each lists the site indices it owns.
   * @return Tuple of (reordered structure, per-sublattice [lower, upper) bounds,
   *         the permutation mapping the new site order back to the original indices).
   */
  template <class T> auto compute_shuffling_bounds(core::structure<T> const& structure,
                                                   std::vector<sublattice> const& composition);

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
   * @brief Evaluate the pair SRO objective for a given bond count. (Hot path.)
   *
   *   O = sum_s sum_{xi <= eta} | a(s, xi, eta) + N^s_{xi,eta} * b(s, xi, eta) |
   *
   * The bond count is read at the canonical cell xi <= eta, where the pair list
   * stored each bond exactly once:
   *   N^s_{xi,eta} = bonds(s, xi, eta) + bonds(s, eta, xi)   for xi < eta
   *   N^s_{xi,xi}  = bonds(s, xi, xi)
   *
   * @tparam T  Floating-point scalar type.
   * @param bonds   Bond counts per cell (see count_pairs).
   * @param coeffs  Precomputed coefficients a, b (see pair_coefficients).
   * @return The scalar objective value O; lower is closer to the target.
   */
  template <class T> T compute_pair_objective(const pair_tensor_t<count_t>& bonds,
                                              const pair_objective_coefficients<T>& coeffs);

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
