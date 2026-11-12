//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_PAIR_TARGETS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_PAIR_TARGETS_IMPL_HPP

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

#include "sqsgen/configuration/pair_targets.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/impl/configuration/shared_impl.hpp"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;

  namespace detail {

    inline constexpr std::string_view pair_targets_key = "pair_targets";

    inline configuration_error pair_targets_error(error_code code, std::string message) {
      return configuration_error{std::string(pair_targets_key), code, std::move(message)};
    }

    /**
     * @brief Pair targets of one (sub)lattice as a sparse map; missing entries mean target 0.
     *
     * No input yields an empty map (every target 0). A single number applies that target to every
     * selected shell and every species pair xi <= eta. A list sets the listed entries; an entry
     * without a shell applies to every selected shell, an entry with a shell overrides it there.
     * Only shells selected in the pair weights can carry a target. Keys are canonical: species
     * sorted by atomic number (xi <= eta), with real shell numbers.
     *
     * @param where     Error-message prefix, or std::nullopt for none (set by lift()).
     * @param input     The target spec: nothing, a single number, or a list of entries.
     * @param structure The (sub)lattice structure, for the species that occur.
     * @param weights   Selected pair weights; only their shells may carry a target.
     */
    template <class T> result_t<pair_sro_t<T>> validate_pair_targets(
        std::optional<std::string_view> where,
        const std::optional<std::variant<T, std::vector<pair_target_input<T>>>>& input,
        const core::structure<T>& structure, const std::map<shell_t, T>& weights) {
      using result = result_t<pair_sro_t<T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(
            pair_targets_error(code, std::format("{}{}", where.value_or(""), message)));
      };

      if (!input.has_value()) return pair_sro_t<T>{};  // every target 0

      const core::sorted_vector<specie_t> present(structure.species);
      const auto selected_shells = weights | std::views::keys;

      // symbol or atomic number -> atomic number that occurs in the (sub)lattice
      const auto resolve = [&](const specie_input& s) -> result_t<specie_t> {
        return validate_species(pair_targets_key, s, where)
            .and_then([&](specie_t z) -> result_t<specie_t> {
              if (!present.contains(z))
                return std::unexpected(
                    pair_targets_error(error_code::not_found,
                                       std::format("{}species {} does not occur in the composition",
                                                   where.value_or(""), z)));
              return z;
            });
      };

      // one number: every selected shell, every species pair xi <= eta
      const auto for_everything = [&](T alpha) -> result {
        if (!std::isfinite(alpha))
          return fail(error_code::bad_argument,
                      std::format("target {} must be finite and <= 1", alpha));
        pair_sro_t<T> out;
        for (const shell_t shell : selected_shells)
          for (std::size_t i = 0; i < present.size(); ++i)
            for (std::size_t j = i; j < present.size(); ++j)
              out.emplace(core::pair_key{shell, {present.at(i), present.at(j)}}, alpha);
        return out;
      };

      const auto from_entries = [&](const std::vector<pair_target_input<T>>& entries) -> result {
        if (entries.empty())
          return fail(error_code::invalid_size, "must not be empty; omit the key for targets of 0");

        pair_sro_t<T> out;
        std::map<std::array<specie_t, 2>, T> for_all_shells;  // entries without shell

        for (const auto& entry : entries) {
          auto xi = resolve(entry.species[0]);
          if (!xi) return std::unexpected(std::move(xi.error()));
          auto eta = resolve(entry.species[1]);
          if (!eta) return std::unexpected(std::move(eta.error()));

          // the pair_key constructor stores the species in canonical order (xi <= eta); reuse that
          // ordering for the "all shells" bucket and the error name so they match the stored keys
          const core::pair_key key{entry.shell.value_or(shell_t{0}),
                                   {std::move(xi).value(), std::move(eta).value()}};
          const std::string name = std::format("{}-{}", key.species[0], key.species[1]);

          if (!entry.shell.has_value()) {
            if (!for_all_shells.emplace(key.species, entry.alpha).second)
              return fail(error_code::bad_argument,
                          std::format("{} without shell is listed more than once", name));
            continue;
          }

          if (!weights.contains(key.shell))
            return fail(error_code::not_found,
                        std::format("{} in shell {}: shell {} is not selected in pair_weights",
                                    name, key.shell, key.shell));
          if (!out.emplace(key, entry.alpha).second)
            return fail(error_code::not_found,
                        std::format("{} in shell {} is listed more than once", name, key.shell));
        }

        // entries without shell: every selected shell, unless an entry with that shell exists
        for (const auto& [species, alpha] : for_all_shells)
          for (const shell_t shell : selected_shells)
            out.try_emplace(core::pair_key{shell, species}, alpha);

        return out;
      };

      return std::visit(helpers::overloaded{for_everything, from_entries}, input.value());
    }

  }  // namespace detail

  template <class T, sublattice_mode Mode>
  result_t<pair_targets_t<T, Mode>> validate_pair_targets_input(
      const pair_targets_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& present,
      const pair_weights_t<T, Mode>& weights) {
    return detail::lift<Mode>(
        detail::pair_targets_key,
        [](std::optional<std::string_view> where, const auto& in, const auto& structure,
           const auto& w) { return detail::validate_pair_targets<T>(where, in, structure, w); },
        detail::distribute<Mode>(input, detail::num_sublattices<Mode>(weights)), present, weights);
  }

  template <class T, sublattice_mode Mode>
  pair_targets_input<T, Mode> to_pair_targets_input(const pair_targets_t<T, Mode>& targets) {
    const auto one = [](const pair_sro_t<T>& sro)
        -> std::optional<std::variant<T, std::vector<pair_target_input<T>>>> {
      std::vector<pair_target_input<T>> out;
      out.reserve(sro.size());
      for (const auto& [key, alpha] : sro)
        out.push_back(pair_target_input<T>{std::make_optional(key.shell),
                                           {specie_input{static_cast<int>(key.species[0])},
                                            specie_input{static_cast<int>(key.species[1])}},
                                           alpha});
      return out;
    };
    return pair_targets_input<T, Mode>{detail::map_sublattices<Mode>(targets, one)};
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_PAIR_TARGETS_IMPL_HPP
