//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_PAIR_WEIGHTS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_PAIR_WEIGHTS_IMPL_HPP

#include <cassert>
#include <cstddef>
#include <format>
#include <map>
#include <optional>
#include <span>
#include <string>

#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace detail {

    inline configuration_error pair_weights_error(error_code code, std::string message) {
      return configuration_error{"pair_weights", code, std::move(message)};
    }

    // Pair weights of one (sub)lattice: selected shells with weights w_i > 0.
    //   no input -> every shell of shell_radii, w_i = r_1 / r_i
    //   input    -> exactly the listed shells; error if a shell does not exist,
    //               is listed twice, or has a negative weight. Shells with w = 0 are dropped.
    // shell_radii[0] = 0 is the site itself, shell 1 is the nearest-neighbour shell.
    // where: prefix for error messages ("" or "sublattice i: ")
    template <class T> result_t<std::map<shell_t, T>> validate_pair_weights(
        const std::optional<std::vector<pair_weight_input<T>>>& input,
        const std::vector<T>& shell_radii, std::string_view where) {
      using result = result_t<std::map<shell_t, T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(pair_weights_error(code, std::format("{}{}", where, message)));
      };

      if (shell_radii.size() < 2) return fail("no coordination shells available");
      const std::size_t num_shells = shell_radii.size() - 1;

      std::map<shell_t, T> out;

      // defaults: w_i = r_1 / r_i
      if (!input) {
        const T r_1 = shell_radii[1];
        for (std::size_t i = 1; i <= num_shells; ++i)
          out.emplace(static_cast<shell_t>(i), r_1 / shell_radii[i]);
        return out;
      }

      if (input->empty()) return fail("must not be empty; omit the key to use the defaults");

      std::set<shell_t> seen;
      for (const auto& [shell, weight] : *input) {
        if (shell == 0) return fail("shell 0 is the site itself, the first shell is 1");
        if (shell > num_shells)
          return fail(
              error_code::not_found,
              std::format("shell {} does not exist, there are {} shells", shell, num_shells));
        if (!seen.insert(shell).second)
          return fail(error_code::bad_argument,
                      std::format("shell {} is listed more than once", shell));
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
    if constexpr (Mode == sublattice_mode::interact) {
      // key missing or null -> defaults
      return detail::validate_pair_weights<T>(input ? *input : std::nullopt, shell_radii, "");
    } else {
      // split: one entry per sublattice; key missing -> defaults everywhere,
      // entry null -> defaults for that sublattice
      const std::size_t num_sublattices = shell_radii.size();
      if (input && input->size() != num_sublattices)
        return std::unexpected(detail::pair_weights_error(
            error_code::invalid_size, std::format("{} entries given, but there are {} sublattices",
                                                  input->size(), num_sublattices)));

      pair_weights_t<T, Mode> out;
      out.reserve(num_sublattices);
      for (std::size_t i = 0; i < num_sublattices; ++i) {
        auto weights = detail::validate_pair_weights<T>(
            input ? (*input)[i] : std::nullopt, shell_radii[i], std::format("sublattice {}: ", i));
        if (!weights) return std::unexpected(std::move(weights.error()));
        out.push_back(std::move(*weights));
      }
      return out;
    }
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_PAIR_WEIGHTS_IMPL_HPP
