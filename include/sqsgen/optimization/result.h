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

  /// Canonical triplet key: sorted shells (s1 <= s2 <= s3) with the species in matching order.
  struct triplet_key {
    std::array<std::uint32_t, 3> shells;
    std::array<specie_t, 3> species;
    auto operator<=>(const triplet_key&) const = default;
  };

  template <class T> using pair_sro_t = std::map<pair_key, T>;
  template <class T> using triplet_sro_t = std::map<triplet_key, T>;

  template <class T> pair_sro_t<T> to_pair_sro(const pair_tensor_t<T>& sro);

  template <class T>
  std::vector<pair_sro_t<T>> to_pair_sro(const std::vector<pair_tensor_t<T>>& sro);

  /**
   * @brief Convert a dense, symmetrized triplet-SRO tensor into a sparse canonical map.
   *
   * A cell is canonical iff the species are ordered within positions that share a shell:
   * s1 == s2 => x1 <= x2 and s2 == s3 => x2 <= x3. Each class stores its sorted shells
   * (s1 <= s2 <= s3) with the species in matching order.
   *
   * @tparam T  Floating-point scalar type.
   * @param beta     Dense tensor indexed (class, x1, x2, x3), shape (num_classes, K, K, K).
   * @param classes  classes[c] = the sorted shells of class c.
   * @return Map from triplet_key{shells, {x1, x2, x3}} to the SRO value.
   */
  template <class T>
  triplet_sro_t<T> to_triplet_sro(const triplet_tensor_t<T>& beta,
                                  const std::vector<std::array<std::uint32_t, 3>>& classes);

  /**
   * @brief Split-mode overload: convert one triplet-SRO tensor per sublattice.
   *
   * @tparam T  Floating-point scalar type.
   * @param beta     One dense triplet-SRO tensor per sublattice.
   * @param classes  The class table for each sublattice (classes[i] matches beta[i]).
   * @return One canonical map per sublattice, in the same order.
   */
  template <class T> std::vector<triplet_sro_t<T>> to_triplet_sro(
      const std::vector<triplet_tensor_t<T>>& beta,
      const std::vector<std::vector<std::array<std::uint32_t, 3>>>& classes);

  template <class T, sublattice_mode Mode, class Sro> struct order_result {
    sublattice_mode_nesting<T, Mode> objective{};
    sublattice_mode_nesting<Sro, Mode> sro;
  };

  template <class T, sublattice_mode Mode> struct sqs_result {
    sublattice_mode_nesting<configuration_t, Mode> configuration;
    std::optional<order_result<T, Mode, pair_sro_t<T>>> pairs;
    std::optional<order_result<T, Mode, triplet_sro_t<T>>> triplets;

    bool operator==(const sqs_result& o) const { return configuration == o.configuration; }

    [[nodiscard]] T total_objective() const;
  };

  struct sqs_result_hash;

};  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/result_impl.hpp"

#endif
