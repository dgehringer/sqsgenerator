//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_ENERGY_OBJECTIVE_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_ENERGY_OBJECTIVE_IMPL_HPP

#include <expected>
#include <optional>
#include <utility>

#include "sqsgen/configuration/energy_objective.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;

  template <class T, sublattice_mode Mode>
  result_t<energy_objective<T, Mode>> validate_energy_objective_input(
      const energy_objective_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      bool pairs, bool triplets) {
    std::optional<pair_interactions_t<T, Mode>> validated_pairs;
    if (pairs) {
      if (!input.pairs.has_value())
        return std::unexpected(configuration_error{
            "pair_interactions", error_code::not_found,
            "pair interactions are required when pairs are enabled (no default is computed)"});
      auto p = validate_pair_interactions_input<T, Mode>(input.pairs.value(), structure);
      if (!p) return std::unexpected(std::move(p).error());
      validated_pairs = std::move(p).value();
    }

    std::optional<triplet_interactions_t<T, Mode>> validated_triplets;
    if (triplets) {
      if (!input.triplets.has_value())
        return std::unexpected(
            configuration_error{"triplet_interactions", error_code::not_found,
                                "triplet interactions are required when triplets are enabled (no "
                                "default is computed)"});
      auto t = validate_triplet_interactions_input<T, Mode>(input.triplets.value(), structure,
                                                            triplet_classes);
      if (!t) return std::unexpected(std::move(t).error());
      validated_triplets = std::move(t).value();
    }

    return energy_objective<T, Mode>{.pairs = std::move(validated_pairs),
                                     .triplets = std::move(validated_triplets)};
  }

  template <class T, sublattice_mode Mode>
  energy_objective_input<T, Mode> to_energy_objective_input(const energy_objective<T, Mode>& obj) {
    return energy_objective_input<T, Mode>{.pairs = obj.pairs.transform([](const auto& p) {
                                             return to_pair_interactions_input<T, Mode>(p);
                                           }),
                                           .triplets = obj.triplets.transform([](const auto& t) {
                                             return to_triplet_interactions_input<T, Mode>(t);
                                           })};
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_ENERGY_OBJECTIVE_IMPL_HPP
