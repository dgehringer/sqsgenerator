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

    // Canonical form of a triangle: the three corners as (opposite shell, species) pairs,
    // sorted by shell, then by species. Corners with equal opposite shell are interchangeable,
    // so sorting their species makes every equivalent spelling give the same key
    // (same rule as make_triplet_list).
    inline core::triplet_key canonical_triplet(const shell_triple& shells,
                                               const std::array<specie_t, 3>& species) {
      std::array<std::pair<shell_t, specie_t>, 3> corners{{
          {shells[0], species[0]},
          {shells[1], species[1]},
          {shells[2], species[2]},
      }};
      std::ranges::sort(corners);
      return core::triplet_key{
          .shells = {corners[0].first, corners[1].first, corners[2].first},
          .species = {corners[0].second, corners[1].second, corners[2].second},
      };
    }

    // "(1, 1, 2)/(13, 13, 26)" for error messages
    inline std::string to_string(const core::triplet_key& key) {
      return std::format("({}, {}, {})/({}, {}, {})", key.shells[0], key.shells[1], key.shells[2],
                         key.species[0], key.species[1], key.species[2]);
    }

    // Triplet targets of one (sub)lattice as a sparse map; missing entries mean target 0.
    //   no input -> empty map (every target 0)
    //   number   -> that target for every selected class and every canonical species tuple
    //   list     -> listed entries; the class (sorted shells) must be selected in
    //               triplet_weights
    // Keys are canonical (canonical_triplet), with real shell numbers and atomic numbers.
    // where: prefix for error messages ("" or "sublattice i: ")
    template <class T> result_t<triplet_sro_t<T>> validate_triplet_targets(
        const std::optional<std::variant<T, std::vector<triplet_target_input<T>>>>& input,
        const core::structure<T>& structure, const std::map<shell_triple, T>& weights,
        std::string_view where) {
      using result = result_t<triplet_sro_t<T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(triplet_targets_error(code, std::format("{}{}", where, message)));
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
                    std::format("{}species {} does not occur in the composition", where, z)));
              return z;
            });
      };

      // one number: every selected class, every species tuple; tuples that are equivalent
      // under the class symmetry collapse to one canonical key (emplace keeps the first)
      const auto for_everything = [&](T beta) -> result {
        if (!std::isfinite(beta))
          return fail(error_code::bad_argument, std::format("target {} must be finite", beta));
        triplet_sro_t<T> out;
        for (const shell_triple& shells : weights | std::views::keys)
          for (const specie_t xi : present)
            for (const specie_t eta : present)
              for (const specie_t zeta : present)
                out.emplace(canonical_triplet(shells, {xi, eta, zeta}), beta);
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

          const core::triplet_key key = canonical_triplet(entry.shells, species);
          const std::string name = to_string(key);

          if (!weights.contains(key.shells))
            return fail(std::format("{}: class ({}, {}, {}) is not selected in triplet_weights",
                                    name, key.shells[0], key.shells[1], key.shells[2]));
          if (!std::isfinite(entry.beta))
            return fail(std::format("target of {} must be finite", name));
          if (!out.emplace(key, entry.beta).second)
            return fail(std::format("{} is listed more than once", name));
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
    if constexpr (Mode == sublattice_mode::interact) {
      // key missing or null -> every target 0
      return detail::validate_triplet_targets<T>(input.value_or(std::nullopt), present, weights,
                                                 "");
    } else {
      // split: one entry per sublattice; key missing -> targets 0 everywhere,
      // entry null -> targets 0 for that sublattice
      const std::size_t num_sublattices = weights.size();
      if (present.size() != num_sublattices)
        return std::unexpected(detail::triplet_targets_error(std::format(
            "{} structures, but {} triplet weight maps", present.size(), num_sublattices)));
      if (input.has_value() && input.value().size() != num_sublattices)
        return std::unexpected(detail::triplet_targets_error(
            std::format("{} entries given, but there are {} sublattices", input.value().size(),
                        num_sublattices)));

      triplet_targets_t<T, Mode> out;
      out.reserve(num_sublattices);
      for (std::size_t i = 0; i < num_sublattices; ++i) {
        auto targets = detail::validate_triplet_targets<T>(
            input.has_value() ? input.value()[i] : std::nullopt, present[i], weights[i],
            std::format("sublattice {}: ", i));
        if (!targets) return std::unexpected(std::move(targets.error()));
        out.push_back(std::move(targets.value()));
      }
      return out;
    }
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_TRIPLET_TARGETS_IMPL_HPP
