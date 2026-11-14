//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_PAIR_INTERACTIONS_H
#define SQSGEN_CONFIGURATION_PAIR_INTERACTIONS_H

#include <array>
#include <optional>
#include <variant>
#include <vector>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;

  using specie_input = std::variant<int, std::string>;  // atomic number or chemical symbol

  template <class T> struct pair_interaction_input {
    std::optional<shell_t> shell;
    std::array<specie_input, 2> species;
    T interaction;
  };

  template <class T, sublattice_mode Mode> using pair_interactions_input
      = sublattice_mode_nesting<std::vector<pair_interaction_input<T>>, Mode>;

  template <class T, sublattice_mode Mode> using pair_interactions_t
      = sublattice_mode_nesting<pair_sro_t<T>, Mode>;

  template <class T, sublattice_mode Mode>
  result_t<pair_interactions_t<T, Mode>> validate_pair_interactions_input(
      const pair_interactions_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& present);

  template <class T, sublattice_mode Mode>
  pair_interactions_input<T, Mode> to_pair_interactions_input(
      const pair_interactions_t<T, Mode>& interactions);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/pair_interactions_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_PAIR_INTERACTIONS_H
