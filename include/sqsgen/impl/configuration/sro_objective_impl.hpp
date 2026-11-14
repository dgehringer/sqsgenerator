//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_SRO_OBJECTIVE_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_SRO_OBJECTIVE_IMPL_HPP

#include <expected>
#include <optional>

#include "sqsgen/configuration/pair_targets.h"
#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/configuration/sro_objective.h"
#include "sqsgen/configuration/triplet_weights.h"
#include "sqsgen/core/helpers/expected.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;

  template <class T, sublattice_mode Mode> result_t<sro_pairs<T, Mode>> validate_sro_pairs_input(
      const sro_pairs_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const shell_radii_t<T, Mode>& shell_radii) {
    const auto weights = validate_pair_weights_input<T, Mode>(input.weights, shell_radii);
    if (!weights) return std::unexpected(std::move(weights).error());
    const auto targets
        = validate_pair_targets_input<T, Mode>(input.targets, structure, weights.value());
    if (!targets) return std::unexpected(std::move(targets).error());
    return sro_pairs<T, Mode>{.weights = std::move(weights).value(),
                              .targets = std::move(targets).value()};
  }

  template <class T, sublattice_mode Mode>
  result_t<sro_triplets<T, Mode>> validate_sro_triplets_input(
      const sro_triplets_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const shell_radii_t<T, Mode>& shell_radii) {
    const auto weights
        = validate_triplet_weights_input<T, Mode>(input.weights, triplet_classes, shell_radii);
    if (!weights) return std::unexpected(std::move(weights).error());
    const auto targets
        = validate_triplet_targets_input<T, Mode>(input.targets, structure, weights.value());
    if (!targets) return std::unexpected(std::move(targets).error());
    return sro_triplets<T, Mode>{.weights = std::move(weights).value(),
                                 .targets = std::move(targets).value()};
  }

  template <class T, sublattice_mode Mode> result_t<sro_objective<T, Mode>> validate_sro_input(
      const sro_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const shell_radii_t<T, Mode>& shell_radii) {
    using result_type = result_t<sro_objective<T, Mode>>;
    return helpers::combine(
               validate_sro_pairs_input<T, Mode>(
                   input.pairs.value_or(sro_pairs_input<T, Mode>::empty()), structure, shell_radii),
               validate_sro_triplets_input(
                   input.triplets.value_or(sro_triplets_input<T, Mode>::empty()), structure,
                   triplet_classes, shell_radii))
        .and_then([](auto&& pairs_and_triplets) -> result_type {
          auto& [pairs, triplets] = pairs_and_triplets;
          return sro_objective<T, Mode>{
              .pairs = std::move(pairs), .triplets = std::move(triplets), .lambda = std::nullopt};
        });
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_SRO_OBJECTIVE_IMPL_HPP
