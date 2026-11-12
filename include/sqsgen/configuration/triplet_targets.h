//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_TRIPLET_TARGETS_H
#define SQSGEN_CONFIGURATION_TRIPLET_TARGETS_H

#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "sqsgen/configuration/triplet_weights.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  using specie_input = std::variant<int, std::string>;

  template <class T> struct triplet_target_input {
    shell_triple shells;                  // shells[k] = shell of the edge opposite corner k
    std::array<specie_input, 3> species;  // species[k] = species at corner k
    T beta;
  };

  template <class T, sublattice_mode Mode> using triplet_targets_t
      = sublattice_mode_nesting<triplet_sro_t<T>, Mode>;

  template <class T, sublattice_mode Mode> using triplet_targets_input
      = std::optional<sublattice_mode_nesting<
          std::optional<std::variant<T, std::vector<triplet_target_input<T>>>>, Mode>>;

  template <class T, sublattice_mode Mode>
  result_t<triplet_targets_t<T, Mode>> validate_triplet_targets_input(
      const triplet_targets_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& present,
      const triplet_weights_t<T, Mode>& weights);

  template <class T, sublattice_mode Mode> triplet_targets_input<T, Mode> to_triplet_targets_input(
      const triplet_targets_t<T, Mode>& targets);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/triplet_targets_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_TRIPLET_TARGETS_H
