//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_PAIR_WEIGHTS_H
#define SQSGEN_CONFIGURATION_PAIR_WEIGHTS_H

#include <cstddef>
#include <map>

#include "sqsgen/configuration/shell_radii.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T> struct pair_weight_input {
    shell_t shell;
    T weight;
  };

  template <class T, sublattice_mode Mode> using pair_weights_input = std::optional<
      sublattice_mode_nesting<std::optional<std::vector<pair_weight_input<T>>>, Mode>>;

  template <class T, sublattice_mode Mode> using pair_weights_t
      = sublattice_mode_nesting<std::map<shell_t, T>, Mode>;

  template <class T, sublattice_mode Mode>
  result_t<pair_weights_t<T, Mode>> validate_pair_weights_input(
      const pair_weights_input<T, Mode>&,
      const sublattice_mode_nesting<std::vector<T>, Mode>& shell_radii);

  template <class T, sublattice_mode Mode>
  pair_weights_input<T, Mode> to_pair_weights_input(const pair_weights_t<T, Mode>& weights);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/pair_weights_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_PAIR_WEIGHTS_H
