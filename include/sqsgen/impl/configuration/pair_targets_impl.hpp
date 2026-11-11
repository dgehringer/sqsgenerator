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

    // Pair targets of one (sub)lattice as a sparse map; missing entries mean target 0.
    //   no input -> empty map (every target 0)
    //   number   -> that target for every selected shell and every species pair xi <= eta
    //   list     -> listed entries; an entry without shell applies to every selected shell,
    //               an entry with shell overrides it for that shell
    // Only shells selected in the pair weights can carry a target.
    // Keys are canonical: species sorted by atomic number (xi <= eta), real shell numbers.
    // where: prefix for error messages ("" or "sublattice i: ")
    template <class T> result_t<pair_sro_t<T>> validate_pair_targets(
        const std::optional<std::variant<T, std::vector<pair_target_input<T>>>>& input,
        const core::structure<T>& structure, const std::map<shell_t, T>& weights,
        std::string_view where) {
      using result = result_t<pair_sro_t<T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(pair_targets_error(code, std::format("{}{}", where, message)));
      };

      if (!input.has_value()) return pair_sro_t<T>{};  // every target 0

      const core::sorted_vector<specie_t> present(structure.species);
      const auto selected_shells = weights | std::views::keys;

      // symbol or atomic number -> atomic number that occurs in the (sub)lattice
      const auto resolve = [&](const specie_input& s) -> result_t<specie_t> {
        return validate_species(pair_targets_key, s, where)
            .and_then([&](specie_t z) -> result_t<specie_t> {
              if (!present.contains(z))
                return std::unexpected(pair_targets_error(
                    error_code::not_found,
                    std::format("{}species {} does not occur in the composition", where, z)));
              return z;
            });
      };

      // one number: every selected shell, every species pair xi <= eta
      const auto for_everything = [&](T alpha) -> result {
        if (!std::isfinite(alpha))
          return fail(std::format("target {} must be finite and <= 1", alpha));
        pair_sro_t<T> out;
        for (const shell_t shell : selected_shells)
          for (std::size_t i = 0; i < present.size(); ++i)
            for (std::size_t j = i; j < present.size(); ++j)
              out.emplace(core::pair_key{.shell = shell, .species = {present[i], present[j]}},
                          alpha);
        return out;
      };

      // list of entries
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

          std::array<specie_t, 2> species{std::move(xi).value(), std::move(eta).value()};
          std::ranges::sort(species);  // canonical: xi <= eta
          const std::string name = std::format("{}-{}", species[0], species[1]);

          if (!entry.shell.has_value()) {
            if (!for_all_shells.emplace(species, entry.alpha).second)
              return fail(error_code::bad_argument,
                          std::format("{} without shell is listed more than once", name));
            continue;
          }

          const shell_t shell = entry.shell.value();
          if (!weights.contains(shell))
            return fail(error_code::not_found,
                        std::format("{} in shell {}: shell {} is not selected in pair_weights",
                                    name, shell, shell));
          if (!out.emplace(core::pair_key{.shell = shell, .species = species}, entry.alpha).second)
            return fail(error_code::not_found,
                        std::format("{} in shell {} is listed more than once", name, shell));
        }

        // entries without shell: every selected shell, unless an entry with that shell exists
        for (const auto& [species, alpha] : for_all_shells)
          for (const shell_t shell : selected_shells)
            out.try_emplace(core::pair_key{.shell = shell, .species = species}, alpha);

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
    if constexpr (Mode == sublattice_mode::interact) {
      // key missing or null -> every target 0
      return detail::validate_pair_targets<T>(input.value_or(std::nullopt), present, weights, "");
    } else {
      // split: one entry per sublattice; key missing -> targets 0 everywhere,
      // entry null -> targets 0 for that sublattice
      const std::size_t num_sublattices = weights.size();
      if (present.size() != num_sublattices)
        return std::unexpected(detail::pair_targets_error(std::format(
            "{} structures, but {} pair weight maps", present.size(), num_sublattices)));
      if (input.has_value() && input.value().size() != num_sublattices)
        return std::unexpected(
            detail::pair_targets_error(std::format("{} entries given, but there are {} sublattices",
                                                   input.value().size(), num_sublattices)));

      pair_targets_t<T, Mode> out;
      out.reserve(num_sublattices);
      for (std::size_t i = 0; i < num_sublattices; ++i) {
        auto targets = detail::validate_pair_targets<T>(
            input.has_value() ? input.value()[i] : std::nullopt, present[i], weights[i],
            std::format("sublattice {}: ", i));
        if (!targets) return std::unexpected(std::move(targets.error()));
        out.push_back(std::move(targets.value()));
      }
      return out;
    }
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_PAIR_TARGETS_IMPL_HPP
