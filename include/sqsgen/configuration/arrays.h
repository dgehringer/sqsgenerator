//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_ARRAYS_H
#define SQSGEN_CONFIGURATION_ARRAYS_H

#include <optional>
#include <variant>

#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/configuration/shell_radii.h"
#include "sqsgen/configuration/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {
  template <class T, sublattice_mode Mode> using pair_weights_sro_t
      = sublattice_mode_nesting<pair_tensor_t<T>, Mode>;
  template <class T, sublattice_mode Mode> using target_objective_t
      = sublattice_mode_nesting<pair_tensor_t<T>, Mode>;

  template <class T> using array_spec = std::variant<T, core::nested_t<T, 2>, core::nested_t<T, 3>>;

  template <class T, sublattice_mode Mode> using pair_weights_sro_input
      = std::optional<sublattice_mode_nesting<array_spec<T>, Mode>>;

  template <class T, sublattice_mode Mode> using target_objective_input
      = std::optional<sublattice_mode_nesting<array_spec<T>, Mode>>;

  template <class T, sublattice_mode Mode>
  result_t<pair_weights_sro_t<T, Mode>> validate_pair_weights_input(
      const pair_weights_sro_input<T, Mode>&, const structure_t<T, Mode>&,
      const pair_weights_t<T, Mode>&, const shell_radii_t<T, Mode>&);

  template <class T, sublattice_mode Mode>
  result_t<target_objective_t<T, Mode>> validate_target_objective_input(
      const target_objective_input<T, Mode>&, const structure_t<T, Mode>&,
      const pair_weights_t<T, Mode>&, const shell_radii_t<T, Mode>&);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/arrays_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_ARRAYS_H
