//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_TRIPLET_WEIGHTS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_TRIPLET_WEIGHTS_IMPL_HPP

#include <algorithm>
#include <cstddef>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "sqsgen/configuration/triplet_weights.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace detail {
    template <class T> result_t<std::map<shell_triple, T>> validate_triplet_weights(
        const std::optional<std::vector<std::tuple<shell_triple, T>>>& input,
        const core::triplet_class_mapping_t& available, const std::vector<T>& shell_radii,
        std::string_view where) {
      using result = result_t<std::map<shell_triple, T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(
            configuration_error{"triplet_weights", code, std::format("{}{}", where, message)});
      };
      const std::size_t num_shells = shell_radii.size() - 1;  // index 0 = the site itself

      std::map<shell_triple, T> out;
      if (!input) {
        const T r_1 = shell_radii[1];
        // each triple is sorted, so shells[2] is the shell of the longest edge
        for (const auto& shells : available) out.emplace(shells, r_1 / shell_radii[shells[2]]);
      } else {
        if (input->empty()) return fail("must not be empty; omit the key to use the defaults");
        std::set<shell_triple> seen;
        for (auto [shells, weight] : *input) {  // copy: we sort the shells
          std::ranges::sort(shells);
          if (shells[0] == 0) return fail("shell 0 is the site itself, the first shell is 1");
          if (shells[2] > num_shells)
            return fail(std::format("class {}: shell {} does not exist, there are {} shells",
                                    shells, shells[2], num_shells));
          if (!std::ranges::binary_search(available, shells))
            return fail(std::format("class {}: no such triangle occurs in the structure", shells));
          if (!seen.insert(shells).second)
            return fail(std::format("class {} is listed more than once", shells));
          if (!(weight >= T{0}))  // also rejects NaN
            return fail(std::format("weight of {} must be >= 0", shells));
          if (weight > T{0}) out.emplace(shells, weight);
        }
      }
      if (out.empty()) return fail("no triplet class with weight > 0");
      return out;
    }
  }  // namespace detail

  template <class T, sublattice_mode Mode>
  result_t<triplet_weights_t<T, Mode>> validate_triplet_weights_input(
      const triplet_weights_input<T, Mode>& input,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const sublattice_mode_nesting<std::vector<T>, Mode>& shell_radii) {
    if constexpr (Mode == sublattice_mode::interact) {
      return detail::validate_triplet_weights<T>(input ? *input : std::nullopt, triplet_classes,
                                                 shell_radii, "");
    } else {
      const std::size_t num_sublattices = triplet_classes.size();
      if (input && input->size() != num_sublattices)
        return std::unexpected(
            configuration_error{"triplet_weights", error_code::invalid_size,
                                std::format("{} entries given, but there are {} sublattices",
                                            input->size(), num_sublattices)});

      triplet_weights_t<T, Mode> out;
      out.reserve(num_sublattices);
      for (std::size_t i = 0; i < num_sublattices; ++i) {
        auto one = detail::validate_triplet_weights<T>(input ? (*input)[i] : std::nullopt,
                                                       triplet_classes[i], shell_radii[i],
                                                       std::format("sublattice {}: ", i));
        if (!one) return std::unexpected(std::move(one.error()));
        out.push_back(std::move(one).value());
      }
      return out;
    }
  }
}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_TRIPLET_WEIGHTS_IMPL_HPP
