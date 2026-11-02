//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CORE_STRUCTURE_UTILS_H
#define SQSGEN_CORE_STRUCTURE_UTILS_H

#include <cstddef>
#include <limits>
#include <map>
#include <vector>

#include "sqsgen/core/eigen.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  // forward declaration; the full definition lives in sqsgen/core/structure.h
  template <class T> class structure;

  template <class T>
  matrix_t<T> distance_matrix(const lattice_t<T>& lattice, const coords_t<T>& frac_coords);

  template <class T> shell_matrix_t shell_matrix(const matrix_t<T>& distance_matrix,
                                                 const std::vector<T>& dists, T atol, T rtol);

  template <class T> structure<T> supercell(const structure<T>& structure, std::size_t a,
                                            std::size_t b, std::size_t c);

  template <class T> cube_t<T> compute_prefactors(const shell_matrix_t& shell_matrix,
                                                  const std::map<std::size_t, T>& weights,
                                                  const configuration_t& configuration);

  template <class T>
  cube_t<T> compute_prefactors(const structure<T>& structure, const std::vector<T>& shell_radii,
                               const std::map<std::size_t, T>& weights,
                               T atol = std::numeric_limits<T>::epsilon(), T rtol = 1e-9);

  template <class T> std::vector<T> distances_naive(const structure<T>& structure,
                                                    T atol = std::numeric_limits<T>::epsilon(),
                                                    T rtol = 1e-9);

  template <class T>
  std::vector<T> distances_histogram(const structure<T>& structure, T bin_width, T peak_isolation);

}  // namespace sqsgen::core

#include "sqsgen/impl/core/structure_utils_impl.hpp"

#endif  // SQSGEN_CORE_STRUCTURE_UTILS_H
