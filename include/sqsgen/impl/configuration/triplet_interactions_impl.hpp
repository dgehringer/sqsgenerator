//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_TRIPLET_INTERACTIONS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_TRIPLET_INTERACTIONS_IMPL_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "sqsgen/configuration/triplet_interactions.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/impl/configuration/shared_impl.hpp"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;

  namespace detail {

    inline constexpr std::string_view triplet_interactions_key = "triplet_interactions";

    inline configuration_error triplet_interactions_error(error_code code, std::string message) {
      return configuration_error{std::string(triplet_interactions_key), code, std::move(message)};
    }

    template <class T> result_t<triplet_sro_t<T>> validate_triplet_interactions(
        std::optional<std::string_view> where,
        const std::vector<triplet_interaction_input<T>>& input, const core::structure<T>& structure,
        const core::triplet_class_mapping_t& classes) {
      using result = result_t<triplet_sro_t<T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(
            triplet_interactions_error(code, std::format("{}{}", where.value_or(""), message)));
      };

      if (input.empty())
        return fail(error_code::invalid_size, "at least one triplet interaction is required");

      const core::sorted_vector<specie_t> present(structure.species);

      triplet_sro_t<T> out;
      for (const auto& entry : input) {
        std::array<specie_t, 3> species{};
        for (std::size_t k = 0; k < 3; ++k) {
          auto z
              = resolve_present_species(triplet_interactions_key, entry.species[k], present, where);
          if (!z) return std::unexpected(std::move(z).error());
          species[k] = std::move(z).value();
        }

        const core::triplet_key key{entry.shells, species};
        const std::string name = to_string(key);

        if (std::ranges::find(classes, key.shells) == classes.end())
          return fail(error_code::not_found,
                      std::format("{}: class ({}, {}, {}) is not a triplet class of the structure",
                                  name, key.shells[0], key.shells[1], key.shells[2]));
        if (!std::isfinite(entry.interaction))
          return fail(error_code::bad_argument,
                      std::format("interaction of {} must be finite", name));
        if (!out.emplace(key, entry.interaction).second)
          return fail(error_code::bad_argument, std::format("{} is listed more than once", name));
      }
      return out;
    }

  }  // namespace detail

  template <class T, sublattice_mode Mode>
  result_t<triplet_interactions_t<T, Mode>> validate_triplet_interactions_input(
      const triplet_interactions_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes) {
    return detail::lift<Mode>(
        detail::triplet_interactions_key,
        [](std::optional<std::string_view> where, const auto& in, const auto& s, const auto& c) {
          return detail::validate_triplet_interactions<T>(where, in, s, c);
        },
        input, structure, triplet_classes);
  }

  template <class T, sublattice_mode Mode>
  triplet_interactions_input<T, Mode> to_triplet_interactions_input(
      const triplet_interactions_t<T, Mode>& interactions) {
    const auto one = [](const triplet_sro_t<T>& sro) {
      std::vector<triplet_interaction_input<T>> out;
      out.reserve(sro.size());
      for (const auto& [key, value] : sro)
        out.push_back(triplet_interaction_input<T>{key.shells,
                                                   {specie_input{static_cast<int>(key.species[0])},
                                                    specie_input{static_cast<int>(key.species[1])},
                                                    specie_input{static_cast<int>(key.species[2])}},
                                                   value});
      return out;
    };
    return detail::map_sublattices<Mode>(interactions, one);
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_TRIPLET_INTERACTIONS_IMPL_HPP
