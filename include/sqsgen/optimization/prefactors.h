//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_OPTIMIZATION_PREFACTORS_H
#define SQSGEN_OPTIMIZATION_PREFACTORS_H

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  /**
   * @brief Normalization prefactors for the pair SRO parameters.
   *
   *   f^s_{xi,eta} = 2 / (N * M_s * m_{xi,eta} * x_xi * x_eta)
   *
   * with the pair multiplicity m_{xi,eta} (2 off-diagonal, 1 on the diagonal), the
   * coordination number M_s of shell s, the mole fractions x, and N sites. Both cells
   * (xi, eta) and (eta, xi) are filled; a selected shell that contains no sites gets a
   * prefactor of 0 and does not contribute.
   *
   * @tparam T  Floating-point scalar type.
   * @param shell_matrix   Per-site-pair shell assignment.
   * @param weights        Shell -> weight map; its keys select the shells, in order.
   * @param configuration  Species of each site (sets the mole fractions).
   * @return Prefactor tensor indexed (shell, species_i, species_j).
   * @throws std::out_of_range if @p weights is empty (no coordination shells selected).
   */
  template <class T> pair_tensor_t<T> compute_pair_prefactors(const shell_matrix_t& shell_matrix,
                                                              const std::map<shell_t, T>& weights,
                                                              const configuration_t& configuration);

  /**
   * @brief Convenience overload that first builds the shell matrix from a structure.
   *
   * Bins the distances of @p structure into shells with @p shell_radii and the given
   * tolerances, then forwards to the shell-matrix overload.
   *
   * @tparam T  Floating-point scalar type.
   * @param structure    Structure whose distance matrix defines the shells.
   * @param shell_radii  Shell radii used to bin interatomic distances.
   * @param weights      Shell -> weight map; its keys select the shells, in order.
   * @param atol         Absolute tolerance for assigning a distance to a shell.
   * @param rtol         Relative tolerance for assigning a distance to a shell.
   * @return Prefactor tensor indexed (shell, species_i, species_j).
   */
  template <class T> pair_tensor_t<T> compute_pair_prefactors(const core::structure<T>& structure,
                                                              const std::vector<T>& shell_radii,
                                                              const std::map<shell_t, T>& weights,
                                                              T atol, T rtol);

  /**
   * @brief Normalization prefactors for the triplet SRO parameters.
   *
   *   f^c_{xi,eta,zeta} = 1 / (n_c * m^c_{xi,eta,zeta} * x_xi * x_eta * x_zeta)
   *
   * where n_c is the number of triangles of class c in @p triplets and m^c is the
   * triplet multiplicity (the number of distinct species orderings across the
   * interchangeable positions of the class). Filled for every species triple; members
   * of the same multiplicity group share a value.
   *
   * @tparam T  Floating-point scalar type.
   * @param triplets       Triangle list (see make_triplet_list); sets n_c per class.
   * @param classes        Class table mapping each class index to its sorted shells.
   * @param configuration  Species of each site (sets the mole fractions).
   * @return Prefactor tensor indexed (class, x1, x2, x3).
   */
  template <class T>
  triplet_tensor_t<T> compute_triplet_prefactors(const core::triplet_list_t& triplets,
                                                 const core::triplet_class_mapping_t& classes,
                                                 const configuration_t& configuration);

}  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/prefactors_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_PREFACTORS_H
