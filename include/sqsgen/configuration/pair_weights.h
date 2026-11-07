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
#include <string>

#include "sqsgen/configuration/shell_radii.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T> using pair_weight_spec = std::map<std::string, T>;

  template <class T, sublattice_mode Mode> using pair_weights_input
      = std::optional<sublattice_mode_nesting<pair_weight_spec<T>, Mode>>;

  template <class T, sublattice_mode Mode> using pair_weights_t
      = sublattice_mode_nesting<std::map<shell_t, T>, Mode>;

  template <class T, sublattice_mode Mode>
  result_t<pair_weights_t<T, Mode>> validate_shell_weight_input(
      const pair_weights_input<T, Mode>&, const shell_radii_t<T, Mode>& shell_radii);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/pair_weights_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_PAIR_WEIGHTS_H
