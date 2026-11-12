//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_PAIR_WEIGHTS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_PAIR_WEIGHTS_IMPL_HPP

#include <cstddef>
#include <expected>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/impl/configuration/shared_impl.hpp"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace detail {

    inline configuration_error pair_weights_error(error_code code, std::string message) {
      return configuration_error{"pair_weights", code, std::move(message)};
    }

    /**
     * @brief Pair weights of one (sub)lattice: the selected shells with weights w_i > 0.
     *
     * Without input every shell of @p shell_radii gets the default w_i = r_1 / r_i; otherwise
     * exactly the listed shells are kept. A shell that does not exist, is listed twice, or has a
     * negative weight is an error; shells with weight 0 are dropped.
     *
     * @param where       Error-message prefix, or std::nullopt for none (set by lift()).
     * @param input       Per-shell weights, or std::nullopt to use the defaults.
     * @param shell_radii Shell radii; index 0 is the site itself, shell 1 the nearest neighbour.
     */
    template <class T> result_t<std::map<shell_t, T>> validate_pair_weights(
        std::optional<std::string_view> where,
        const std::optional<std::vector<pair_weight_input<T>>>& input,
        const std::vector<T>& shell_radii) {
      using result = result_t<std::map<shell_t, T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(
            pair_weights_error(code, std::format("{}{}", where.value_or(""), message)));
      };

      if (shell_radii.size() < 2)
        return fail(error_code::invalid_size, "no coordination shells available");
      const std::size_t num_shells = shell_radii.size() - 1;

      std::map<shell_t, T> out;

      if (!input) {
        const T r_1 = shell_radii[1];
        for (std::size_t i = 1; i <= num_shells; ++i)
          out.emplace(static_cast<shell_t>(i), r_1 / shell_radii[i]);
        return out;
      }

      if (input->empty())
        return fail(error_code::invalid_size,
                    "must not be empty; omit the key to use the defaults");

      core::sorted_vector<shell_t> seen;
      for (const auto& [shell, weight] : *input) {
        if (shell == 0)
          return fail(error_code::bad_argument, "shell 0 is the site itself, the first shell is 1");
        if (shell > num_shells)
          return fail(
              error_code::not_found,
              std::format("shell {} does not exist, there are {} shells", shell, num_shells));
        if (seen.contains(shell))
          return fail(error_code::bad_argument,
                      std::format("shell {} is listed more than once", shell));
        seen.insert(shell);
        if (!(weight >= T{0}))  // also rejects NaN
          return fail(error_code::out_of_range,
                      std::format("weight of shell {} must be >= 0", shell));
        if (weight > T{0}) out.emplace(shell, weight);
      }
      if (out.empty()) return fail(error_code::bad_argument, "no shell with weight > 0");
      return out;
    }

  }  // namespace detail

  template <class T, sublattice_mode Mode>
  result_t<pair_weights_t<T, Mode>> validate_pair_weights_input(
      const pair_weights_input<T, Mode>& input,
      const sublattice_mode_nesting<std::vector<T>, Mode>& shell_radii) {
    return detail::lift<Mode>(
        "pair_weights",
        [](std::optional<std::string_view> where, const auto& in, const auto& radii) {
          return detail::validate_pair_weights<T>(where, in, radii);
        },
        detail::distribute<Mode>(input, detail::num_sublattices<Mode>(shell_radii)), shell_radii);
  }

  template <class T, sublattice_mode Mode>
  pair_weights_input<T, Mode> to_pair_weights_input(const pair_weights_t<T, Mode>& weights) {
    const auto one
        = [](const std::map<shell_t, T>& w) -> std::optional<std::vector<pair_weight_input<T>>> {
      std::vector<pair_weight_input<T>> out;
      out.reserve(w.size());
      for (const auto& [shell, weight] : w) out.push_back(pair_weight_input<T>{shell, weight});
      return out;
    };
    return pair_weights_input<T, Mode>{detail::map_sublattices<Mode>(weights, one)};
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_PAIR_WEIGHTS_IMPL_HPP
