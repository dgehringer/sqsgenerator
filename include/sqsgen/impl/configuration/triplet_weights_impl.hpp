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
#include <expected>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "sqsgen/configuration/triplet_weights.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/impl/configuration/shared_impl.hpp"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace detail {

    // "(1, 1, 2)"; std::format of ranges is not available in every standard library
    inline std::string to_string(const shell_triple& shells) {
      return std::format("({}, {}, {})", shells[0], shells[1], shells[2]);
    }

    inline configuration_error triplet_weights_error(error_code code, std::string message) {
      return configuration_error{"triplet_weights", code, std::move(message)};
    }

    /**
     * @brief Triplet weights of one (sub)lattice: the selected classes with weights w_c > 0.
     *
     * Without input every class that occurs gets the default w_c = r_1 / r_{s3} (with
     * s1 <= s2 <= s3, so s3 is the shell of the longest edge); otherwise exactly the listed
     * classes are kept. A non-existent shell, a class that does not occur in the structure, a
     * class listed twice, or a negative weight is an error; classes with weight 0 are dropped.
     *
     * @param where           Error-message prefix, or std::nullopt for none (set by lift()).
     * @param input           Per-class weights, or std::nullopt to use the defaults.
     * @param triplet_classes Every class occurring within the shell radii, sorted
     *                        (as returned by make_triplet_list over all shells).
     * @param shell_radii     Shell radii; index 0 is the site itself, shell 1 the nearest
     *                        neighbour.
     */
    template <class T> result_t<std::map<shell_triple, T>> validate_triplet_weights(
        std::optional<std::string_view> where,
        const std::optional<std::vector<triplet_weight_input<T>>>& input,
        const core::triplet_class_mapping_t& triplet_classes, const std::vector<T>& shell_radii) {
      using result = result_t<std::map<shell_triple, T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(
            triplet_weights_error(code, std::format("{}{}", where.value_or(""), message)));
      };

      if (shell_radii.size() < 2)
        return fail(error_code::invalid_size, "no coordination shells available");
      const std::size_t num_shells = shell_radii.size() - 1;

      std::map<shell_triple, T> out;

      if (!input) {
        const T r_1 = shell_radii[1];
        for (const auto& shells : triplet_classes)
          out.emplace(shells, r_1 / shell_radii[shells[2]]);
        return out;  // may be empty if no triangle occurs within the shell radii
      }

      if (input->empty())
        return fail(error_code::invalid_size,
                    "must not be empty; omit the key to use the defaults");

      core::sorted_vector<shell_triple> seen;
      for (auto [shells, weight] : *input) {  // copy: the shells are sorted below
        std::ranges::sort(shells);            // canonical: s1 <= s2 <= s3
        if (shells[0] == 0)
          return fail(error_code::bad_argument, "shell 0 is the site itself, the first shell is 1");
        if (shells[2] > num_shells)
          return fail(error_code::not_found,
                      std::format("class {}: shell {} does not exist, there are {} shells",
                                  to_string(shells), shells[2], num_shells));
        // triplet_classes is sorted (make_triplet_list numbers the classes in sorted order)
        if (!std::ranges::binary_search(triplet_classes, shells))
          return fail(
              error_code::not_found,
              std::format("class {}: no such triangle occurs in the structure", to_string(shells)));
        if (seen.contains(shells))
          return fail(error_code::bad_argument,
                      std::format("class {} is listed more than once", to_string(shells)));
        seen.insert(shells);
        if (!(weight >= T{0}))  // also rejects NaN
          return fail(error_code::out_of_range,
                      std::format("weight of class {} must be >= 0", to_string(shells)));
        if (weight > T{0}) out.emplace(shells, weight);
      }
      if (out.empty()) return fail(error_code::bad_argument, "no triplet class with weight > 0");
      return out;
    }

  }  // namespace detail

  template <class T, sublattice_mode Mode>
  result_t<triplet_weights_t<T, Mode>> validate_triplet_weights_input(
      const triplet_weights_input<T, Mode>& input,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const sublattice_mode_nesting<std::vector<T>, Mode>& shell_radii) {
    return detail::lift<Mode>(
        "triplet_weights",
        [](std::optional<std::string_view> where, const auto& in, const auto& classes,
           const auto& radii) {
          return detail::validate_triplet_weights<T>(where, in, classes, radii);
        },
        detail::distribute<Mode>(input, detail::num_sublattices<Mode>(triplet_classes)),
        triplet_classes, shell_radii);
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_TRIPLET_WEIGHTS_IMPL_HPP
