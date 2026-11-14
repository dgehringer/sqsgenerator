//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_ENERGY_OBJECTIVE_H
#define SQSGEN_CONFIGURATION_ENERGY_OBJECTIVE_H

#include <optional>

#include "sqsgen/configuration/pair_interactions.h"
#include "sqsgen/configuration/triplet_interactions.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;

  template <class T, sublattice_mode Mode> struct energy_objective {
    std::optional<pair_interactions_t<T, Mode>> pairs;
    std::optional<triplet_interactions_t<T, Mode>> triplets;
  };

  template <class T, sublattice_mode Mode> struct energy_objective_input {
    std::optional<pair_interactions_input<T, Mode>> pairs;
    std::optional<triplet_interactions_input<T, Mode>> triplets;
  };

  template <class T, sublattice_mode Mode>
  result_t<energy_objective<T, Mode>> validate_energy_objective_input(
      const energy_objective_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      bool pairs, bool triplets);

  template <class T, sublattice_mode Mode>
  energy_objective_input<T, Mode> to_energy_objective_input(const energy_objective<T, Mode>&);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/energy_objective_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_ENERGY_OBJECTIVE_H
