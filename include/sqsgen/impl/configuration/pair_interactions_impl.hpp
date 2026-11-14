//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_PAIR_INTERACTIONS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_PAIR_INTERACTIONS_IMPL_HPP

#include <array>
#include <cmath>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "sqsgen/configuration/pair_interactions.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/impl/configuration/shared_impl.hpp"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;

  namespace detail {

    inline constexpr std::string_view pair_interactions_key = "pair_interactions";

    inline configuration_error pair_interactions_error(error_code code, std::string message) {
      return configuration_error{std::string(pair_interactions_key), code, std::move(message)};
    }

    template <class T> result_t<pair_sro_t<T>> validate_pair_interactions(
        std::optional<std::string_view> where, const std::vector<pair_interaction_input<T>>& input,
        const core::structure<T>& structure) {
      using result = result_t<pair_sro_t<T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(
            pair_interactions_error(code, std::format("{}{}", where.value_or(""), message)));
      };

      if (input.empty())
        return fail(error_code::invalid_size, "at least one pair interaction is required");

      const core::sorted_vector<specie_t> present(structure.species);

      pair_sro_t<T> out;
      for (const auto& entry : input) {
        if (!entry.shell.has_value())
          return fail(error_code::bad_argument,
                      "every pair interaction needs an explicit shell (no propagation)");

        auto xi = resolve_present_species(pair_interactions_key, entry.species[0], present, where);
        if (!xi) return std::unexpected(std::move(xi).error());
        auto eta = resolve_present_species(pair_interactions_key, entry.species[1], present, where);
        if (!eta) return std::unexpected(std::move(eta).error());

        const core::pair_key key{entry.shell.value(),
                                 {std::move(xi).value(), std::move(eta).value()}};
        const std::string name = std::format("{}-{}", key.species[0], key.species[1]);

        if (!std::isfinite(entry.interaction))
          return fail(error_code::bad_argument,
                      std::format("interaction of {} in shell {} must be finite", name, key.shell));
        if (!out.emplace(key, entry.interaction).second)
          return fail(error_code::bad_argument,
                      std::format("{} in shell {} is listed more than once", name, key.shell));
      }
      return out;
    }

  }  // namespace detail

  template <class T, sublattice_mode Mode>
  result_t<pair_interactions_t<T, Mode>> validate_pair_interactions_input(
      const pair_interactions_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& present) {
    return detail::lift<Mode>(
        detail::pair_interactions_key,
        [](std::optional<std::string_view> where, const auto& in, const auto& structure) {
          return detail::validate_pair_interactions<T>(where, in, structure);
        },
        input, present);
  }

  template <class T, sublattice_mode Mode>
  pair_interactions_input<T, Mode> to_pair_interactions_input(
      const pair_interactions_t<T, Mode>& interactions) {
    const auto one = [](const pair_sro_t<T>& sro) {
      std::vector<pair_interaction_input<T>> out;
      out.reserve(sro.size());
      for (const auto& [key, value] : sro)
        out.push_back(pair_interaction_input<T>{std::make_optional(key.shell),
                                                {specie_input{static_cast<int>(key.species[0])},
                                                 specie_input{static_cast<int>(key.species[1])}},
                                                value});
      return out;
    };
    return detail::map_sublattices<Mode>(interactions, one);
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_PAIR_INTERACTIONS_IMPL_HPP
