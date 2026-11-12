//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_TRIPLET_TARGETS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_TRIPLET_TARGETS_IMPL_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <expected>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "sqsgen/configuration/triplet_targets.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/impl/configuration/shared_impl.hpp"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace detail {

    inline constexpr std::string_view triplet_targets_key = "triplet_targets";

    inline configuration_error triplet_targets_error(error_code code, std::string message) {
      return configuration_error{std::string(triplet_targets_key), code, std::move(message)};
    }

    // "(1, 1, 2)/(13, 13, 26)" for error messages
    inline std::string to_string(const core::triplet_key& key) {
      return std::format("({}, {}, {})/({}, {}, {})", key.shells[0], key.shells[1], key.shells[2],
                         key.species[0], key.species[1], key.species[2]);
    }

    /**
     * @brief Triplet targets of one (sub)lattice as a sparse map; missing entries mean target 0.
     *
     * No input yields an empty map (every target 0). A single number applies that target to every
     * selected class and every canonical species tuple. A list sets the listed entries; each
     * entry's class (its sorted shells) must be selected in triplet_weights. Keys are canonical
     * (the triplet_key constructor sorts the corners), with real shell numbers and atomic numbers.
     *
     * @param where     Error-message prefix, or std::nullopt for none (set by lift()).
     * @param input     The target spec: nothing, a single number, or a list of entries.
     * @param structure The (sub)lattice structure, for the species that occur.
     * @param weights   Selected triplet weights; only their classes may carry a target.
     */
    template <class T> result_t<triplet_sro_t<T>> validate_triplet_targets(
        std::optional<std::string_view> where,
        const std::optional<std::variant<T, std::vector<triplet_target_input<T>>>>& input,
        const core::structure<T>& structure, const std::map<shell_triple, T>& weights) {
      using result = result_t<triplet_sro_t<T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(
            triplet_targets_error(code, std::format("{}{}", where.value_or(""), message)));
      };

      if (!input.has_value()) return triplet_sro_t<T>{};  // every target 0

      const core::sorted_vector<specie_t> present(structure.species);

      // symbol or atomic number -> atomic number that occurs in the (sub)lattice
      const auto resolve = [&](const specie_input& s) -> result_t<specie_t> {
        return validate_species(triplet_targets_key, s, where)
            .and_then([&](specie_t z) -> result_t<specie_t> {
              if (!present.contains(z))
                return std::unexpected(triplet_targets_error(
                    error_code::not_found,
                    std::format("{}species {} does not occur in the composition",
                                where.value_or(""), z)));
              return z;
            });
      };

      const auto for_everything = [&](T beta) -> result {
        if (!std::isfinite(beta))
          return fail(error_code::bad_argument, std::format("target {} must be finite", beta));
        triplet_sro_t<T> out;
        for (const shell_triple& shells : weights | std::views::keys)
          for (const specie_t xi : present)
            for (const specie_t eta : present)
              for (const specie_t zeta : present)
                out.emplace(core::triplet_key{shells, {xi, eta, zeta}}, beta);
        return out;
      };

      const auto from_entries = [&](const std::vector<triplet_target_input<T>>& entries) -> result {
        if (entries.empty())
          return fail(error_code::invalid_size, "must not be empty; omit the key for targets of 0");

        triplet_sro_t<T> out;
        for (const auto& entry : entries) {
          std::array<specie_t, 3> species{};
          for (std::size_t k = 0; k < 3; ++k) {
            auto z = resolve(entry.species[k]);
            if (!z) return std::unexpected(std::move(z.error()));
            species[k] = z.value();
          }

          const core::triplet_key key{entry.shells, species};
          const std::string name = to_string(key);

          if (!weights.contains(key.shells))
            return fail(error_code::not_found,
                        std::format("{}: class ({}, {}, {}) is not selected in triplet_weights",
                                    name, key.shells[0], key.shells[1], key.shells[2]));
          if (!std::isfinite(entry.beta))
            return fail(error_code::bad_argument, std::format("target of {} must be finite", name));
          if (!out.emplace(key, entry.beta).second)
            return fail(error_code::bad_argument, std::format("{} is listed more than once", name));
        }
        return out;
      };

      return std::visit(helpers::overloaded{for_everything, from_entries}, input.value());
    }

  }  // namespace detail

  template <class T, sublattice_mode Mode>
  result_t<triplet_targets_t<T, Mode>> validate_triplet_targets_input(
      const triplet_targets_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& present,
      const triplet_weights_t<T, Mode>& weights) {
    return detail::lift<Mode>(
        detail::triplet_targets_key,
        [](std::optional<std::string_view> where, const auto& in, const auto& structure,
           const auto& w) { return detail::validate_triplet_targets<T>(where, in, structure, w); },
        detail::distribute<Mode>(input, detail::num_sublattices<Mode>(weights)), present, weights);
  }

  template <class T, sublattice_mode Mode> triplet_targets_input<T, Mode> to_triplet_targets_input(
      const triplet_targets_t<T, Mode>& targets) {
    const auto one = [](const triplet_sro_t<T>& sro)
        -> std::optional<std::variant<T, std::vector<triplet_target_input<T>>>> {
      std::vector<triplet_target_input<T>> out;
      out.reserve(sro.size());
      for (const auto& [key, beta] : sro)
        out.push_back(triplet_target_input<T>{key.shells,
                                              {specie_input{static_cast<int>(key.species[0])},
                                               specie_input{static_cast<int>(key.species[1])},
                                               specie_input{static_cast<int>(key.species[2])}},
                                              beta});
      return out;
    };
    return triplet_targets_input<T, Mode>{detail::map_sublattices<Mode>(targets, one)};
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_TRIPLET_TARGETS_IMPL_HPP
