//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_SRO_OBJECTIVE_H
#define SQSGEN_CONFIGURATION_SRO_OBJECTIVE_H

#include <optional>

#include "sqsgen/configuration/pair_targets.h"
#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/configuration/shell_radii.h"
#include "sqsgen/configuration/triplet_targets.h"
#include "sqsgen/configuration/triplet_weights.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;

  template <class T, sublattice_mode Mode> struct sro_pairs {
    pair_weights_t<T, Mode> weights;
    pair_targets_t<T, Mode> targets;
  };

  template <class T, sublattice_mode Mode> struct sro_triplets {
    triplet_weights_t<T, Mode> weights;
    triplet_targets_t<T, Mode> targets;
  };

  template <class T, sublattice_mode Mode> struct sro_objective {
    std::optional<sro_pairs<T, Mode>> pairs;
    std::optional<sro_triplets<T, Mode>> triplets;
    T lambda;
  };

  template <class T, sublattice_mode Mode> struct sro_pairs_input {
    pair_weights_input<T, Mode> weights;
    pair_targets_input<T, Mode> targets;

    static sro_pairs_input empty() { return {std::nullopt, std::nullopt}; }
  };

  template <class T, sublattice_mode Mode> struct sro_triplets_input {
    triplet_weights_input<T, Mode> weights;
    triplet_targets_input<T, Mode> targets;
    static sro_triplets_input empty() { return {std::nullopt, std::nullopt}; }
  };
  template <class T, sublattice_mode Mode> struct sro_input {
    std::optional<sro_pairs_input<T, Mode>> pairs;
    std::optional<sro_triplets_input<T, Mode>> triplets;
    std::optional<T> lambda;
    static sro_input<T, Mode> empty() {
      return sro_input<T, Mode>{.pairs = std::move(sro_pairs_input<T, Mode>::empty()),
                                .triplets = std::move(sro_triplets_input<T, Mode>::empty()),
                                .lambda = std::nullopt};
    }
  };

  template <class T, sublattice_mode Mode> result_t<sro_pairs<T, Mode>> validate_sro_pairs_input(
      const sro_pairs_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const shell_radii_t<T, Mode>& shell_radii);

  template <class T, sublattice_mode Mode>
  result_t<sro_triplets<T, Mode>> validate_sro_triplets_input(
      const sro_triplets_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const shell_radii_t<T, Mode>& shell_radii);

  template <class T, sublattice_mode Mode> result_t<sro_objective<T, Mode>> validate_sro_input(
      const sro_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const shell_radii_t<T, Mode>& shell_radii);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/sro_objective_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_SRO_OBJECTIVE_H
