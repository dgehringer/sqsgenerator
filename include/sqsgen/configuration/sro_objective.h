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
  template <class T, sublattice_mode Mode> struct sro_objective_input {
    std::optional<sro_pairs_input<T, Mode>> pairs;
    std::optional<sro_triplets_input<T, Mode>> triplets;
    std::optional<T> lambda;
    static sro_objective_input<T, Mode> empty() {
      return sro_objective_input<T, Mode>{
          .pairs = std::move(sro_pairs_input<T, Mode>::empty()),
          .triplets = std::move(sro_triplets_input<T, Mode>::empty()),
          .lambda = std::nullopt};
    }
  };

  /**
   * @brief Resolve and validate the pair part of an SRO objective.
   *
   * Two-stage: the pair weights are validated against the selected shells first, then the
   * pair targets against those weights and @p structure. The first failing stage short-circuits.
   *
   * @param structure    structure the pair targets are checked against (e.g. species present).
   * @param shell_radii  shell boundaries the weighted shells must fall within.
   * @return the resolved weights and targets, or the first validation error.
   */
  template <class T, sublattice_mode Mode> result_t<sro_pairs<T, Mode>> validate_sro_pairs_input(
      const sro_pairs_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const shell_radii_t<T, Mode>& shell_radii);

  /**
   * @brief Resolve and validate the triplet part of an SRO objective.
   *
   * Two-stage: the triplet weights are validated against @p triplet_classes and the selected
   * shells first, then the triplet targets against those weights and @p structure. The first
   * failing stage short-circuits.
   *
   * @param structure        structure the triplet targets are checked against.
   * @param triplet_classes  the symmetry-distinct triplet classes the weights and targets are keyed
   * by.
   * @param shell_radii      shell boundaries the weighted shells must fall within.
   * @return the resolved weights and targets, or the first validation error.
   */
  template <class T, sublattice_mode Mode>
  result_t<sro_triplets<T, Mode>> validate_sro_triplets_input(
      const sro_triplets_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const shell_radii_t<T, Mode>& shell_radii);

  /**
   * @brief Resolve and validate a full SRO objective (pairs, triplets, lambda).
   *
   * @p pairs and @p triplets select which terms the objective carries: a selected term with no
   * user input is validated from an empty input (all defaults), an unselected term becomes
   * std::nullopt. @p lambda defaults to 0 when the input omits it.
   *
   * @param structure        structure the targets are checked against.
   * @param triplet_classes  the triplet classes the triplet term is keyed by.
   * @param shell_radii      shell boundaries the weighted shells must fall within.
   * @param pairs            whether the objective includes the pair term.
   * @param triplets         whether the objective includes the triplet term.
   * @return the resolved objective, or the first validation error.
   */
  template <class T, sublattice_mode Mode>
  result_t<sro_objective<T, Mode>> validate_sro_objective_input(
      const sro_objective_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const shell_radii_t<T, Mode>& shell_radii, bool pairs, bool triplets);

  template <class T, sublattice_mode Mode>
  sro_pairs_input<T, Mode> to_sro_pairs_input(const sro_pairs<T, Mode>&);
  template <class T, sublattice_mode Mode>
  sro_triplets_input<T, Mode> to_sro_triplets_input(const sro_triplets<T, Mode>&);
  template <class T, sublattice_mode Mode>
  sro_objective_input<T, Mode> to_sro_objective_input(const sro_objective<T, Mode>&);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/sro_objective_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_SRO_OBJECTIVE_H
