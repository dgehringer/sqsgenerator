//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_PAIR_TARGETS_H
#define SQSGEN_CONFIGURATION_PAIR_TARGETS_H

#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  using specie_input = std::variant<int, std::string>;  // atomic number or chemical symbol

  template <class T> struct pair_target_input {
    std::optional<shell_t> shell;  // std::nullopt = all shells
    std::array<specie_input, 2> species;
    T alpha;
  };

  template <class T, sublattice_mode Mode> using pair_targets_input
      = std::optional<sublattice_mode_nesting<
          std::optional<std::variant<T, std::vector<pair_target_input<T>>>>, Mode>>;

  template <class T, sublattice_mode Mode> using pair_targets_t
      = sublattice_mode_nesting<pair_sro_t<T>, Mode>;

  template <class T, sublattice_mode Mode>
  result_t<pair_targets_t<T, Mode>> validate_pair_targets_input(
      const pair_targets_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& present,
      const pair_weights_t<T, Mode>& weights);

  template <class T, sublattice_mode Mode>
  pair_targets_input<T, Mode> to_pair_targets_input(const pair_targets_t<T, Mode>& targets);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/pair_targets_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_PAIR_TARGETS_H
