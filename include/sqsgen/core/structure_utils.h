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
#include <utility>
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

  template <class T> std::vector<T> distances_naive(const structure<T>& structure,
                                                    T atol = std::numeric_limits<T>::epsilon(),
                                                    T rtol = 1e-9);

  template <class T>
  std::vector<T> distances_histogram(const structure<T>& structure, T bin_width, T peak_isolation);

  /**
   * @brief Build the list of site pairs that fall into a selected shell.
   *
   * Enumerates every unordered site pair (i < j) once and keeps it only if the shell
   * of (i, j) appears in @p weights. Each kept pair stores the dense shell index (the
   * position of its shell within @p weights), not the user shell number.
   *
   * @tparam T  Floating-point scalar type.
   * @param structure  Structure whose shell matrix defines the pairs.
   * @param radii      Shell radii used to bin interatomic distances.
   * @param weights    Shell -> weight map; its keys select which shells are kept.
   * @param atol       Absolute tolerance for assigning a distance to a shell.
   * @param rtol       Relative tolerance for assigning a distance to a shell.
   * @return Flat list of selected pairs, each tagged with its dense shell index.
   */
  template <class T>
  pair_list_t make_pair_list(const structure<T>& structure, const std::vector<T>& radii,
                             const std::map<shell_t, T>& weights, T atol, T rtol);

  /**
   * @brief Build the list of triangles whose three edges all lie in selected shells.
   *
   * Each triangle is stored once. Its three sites are ordered by the shell of the
   * opposite edge (s1 <= s2 <= s3, ties broken by site index for determinism). A
   * triangle's class is the tuple of its sorted opposite-edge shells; the returned
   * class table maps each dense class index to that shell tuple.
   *
   * @tparam T  Floating-point scalar type.
   * @param structure  Structure whose shell matrix defines the edges.
   * @param radii      Shell radii used to bin interatomic distances.
   * @param weights    Shell -> weight map; its keys select which shells are kept.
   * @param atol       Absolute tolerance for assigning a distance to a shell.
   * @param rtol       Relative tolerance for assigning a distance to a shell.
   * @return Pair of (triangle list tagged with dense class index, class table).
   */
  template <class T> std::pair<triplet_list_t, triplet_class_mapping_t> make_triplet_list(
      const structure<T>& structure, const std::vector<T>& radii,
      const std::map<shell_t, T>& weights, T atol, T rtol);

}  // namespace sqsgen::core

#include "sqsgen/impl/core/structure_utils_impl.hpp"

#endif  // SQSGEN_CORE_STRUCTURE_UTILS_H
