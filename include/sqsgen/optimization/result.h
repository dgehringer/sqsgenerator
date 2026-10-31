//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_OPTIMIZATION_RESULT_H
#define SQSGEN_OPTIMIZATION_RESULT_H

#include <array>
#include <cstdint>
#include <map>
#include <optional>

#include "sqsgen/types.h"

namespace sqsgen::optimization {

  struct pair_key {
    std::uint32_t shell;
    std::array<specie_t, 2> species;
    auto operator<=>(const pair_key&) const = default;
  };

  // beta: sorted shells (s1 <= s2 <= s3), species in matching order
  struct triplet_key {
    std::array<std::uint32_t, 3> shells;
    std::array<specie_t, 3> species;
    auto operator<=>(const triplet_key&) const = default;
  };

  template <class T> using pair_sro_t = std::map<pair_key, T>;
  template <class T> using triplet_sro_t = std::map<triplet_key, T>;

  template <class T, sublattice_mode Mode> struct sqs_result {
    sublattice_mode_nesting<T, Mode> objective;
    sublattice_mode_nesting<configuration_t, Mode> configuration;
    std::optional<sublattice_mode_nesting<pair_sro_t<T>, Mode>> pair_sro;
    std::optional<sublattice_mode_nesting<triplet_sro_t<T>, Mode>> triplet_sro;

    bool operator==(const sqs_result& o) const { return configuration == o.configuration; }

    [[nodiscard]] T total_objective() const;
  };

  // Hashes sqs_result by its configuration only (consistent with operator==).
  // Definition lives in impl/optimization/result_impl.hpp.
  struct sqs_result_hash;

};  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/result_impl.hpp"

#endif
