//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_COMPOSITION_H
#define SQSGEN_CONFIGURATION_COMPOSITION_H

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  using indices_t = core::sorted_vector<std::size_t>;

  using site_spec = std::variant<std::vector<int>, std::vector<std::string>, std::string, int>;
  using composition_spec = std::map<std::string, site_spec>;
  using composition_input = std::variant<std::vector<composition_spec>, composition_spec>;

  struct sublattice_helper {
    std::optional<indices_t> sites = std::nullopt;
    composition_t composition;
  };

  result_t<std::vector<sublattice>> validate_composition_input(const composition_input&,
                                                               const configuration_t& conf);

  composition_input to_composition_input(const std::vector<sublattice>& composition);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/composition_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_COMPOSITION_H
