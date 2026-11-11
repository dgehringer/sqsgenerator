//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_TRIPLET_WEIGHTS_H
#define SQSGEN_CONFIGURATION_TRIPLET_WEIGHTS_H

#include <array>
#include <map>
#include <optional>
#include <vector>

#include "sqsgen/types.h"

namespace sqsgen::configuration {
  using shell_triple = std::array<shell_t, 3>;

  template <class T> struct triplet_weight_input {
    shell_triple shells;  // any order, when reading it from user input
    T weight;
  };

  template <class T, sublattice_mode Mode> using triplet_weights_input = std::optional<
      sublattice_mode_nesting<std::optional<std::vector<triplet_weight_input<T>>>, Mode>>;

  template <class T, sublattice_mode Mode> using triplet_weights_t
      = sublattice_mode_nesting<std::map<shell_triple, T>, Mode>;

  /**
   * @brief Validate user-supplied triplet weights against the triangle classes of a structure.
   *
   * Each shell triple is sorted ascending, so entry [2] is the shell of the triangle's
   * longest edge. When @p input is absent, every class present in @p triplet_classes is
   * assigned the default weight @c r_1/r_s3 (first shell radius over the longest-edge shell
   * radius). When present, each listed class must be a triangle that actually occurs, be
   * listed at most once, and carry a weight @c >=0 (NaN rejected); classes with weight 0 are
   * dropped. At least one class must remain with weight > 0.
   *
   * @tparam T     Floating-point scalar type.
   * @tparam Mode  Sublattice mode: @c interact (one definition) or @c split (one per sublattice).
   * @param input           Optional per-class weights; omit a class to fall back to defaults.
   * @param triplet_classes Triangle classes present in the structure (see make_triplet_list).
   * @param shell_radii     Shell radii; index 0 is the site itself, so the first shell is index 1.
   * @return The validated class -> weight map, or a configuration_error describing the problem.
   */
  template <class T, sublattice_mode Mode>
  result_t<triplet_weights_t<T, Mode>> validate_triplet_weights_input(
      const triplet_weights_input<T, Mode>& input,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const sublattice_mode_nesting<std::vector<T>, Mode>& shell_radii);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/triplet_weights_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_TRIPLET_WEIGHTS_H
