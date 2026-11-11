#ifndef SQSGEN_IMPL_CONFIGURATION_PAIR_TARGETS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_PAIR_TARGETS_IMPL_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <expected>
#include <format>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "sqsgen/configuration/pair_targets.h"
#include "sqsgen/core/atom.h"
#include "sqsgen/impl/configuration/shared_impl.hpp"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;

  namespace detail {

    inline configuration_error pair_targets_error(error_code code, std::string message) {
      return configuration_error{"pair_targets", code, std::move(message)};
    }

    // atomic number or chemical symbol -> atomic number that occurs in this (sub)lattice
    // present: distinct atomic numbers of the (sub)lattice
    inline result_t<specie_t> resolve_specie(const specie_input& s,
                                             const std::vector<specie_t>& present,
                                             std::string_view where) {
      const auto ordinal = validate_species("pair_targets", s, std::make_optional(where));
      if (!ordinal) return std::unexpected(std::move(ordinal).error());
      if (std::ranges::find(present, ordinal.value()) == present.end())
        return std::unexpected(pair_targets_error(
            error_code::bad_argument,
            std::format("{}ordinal {} does not occur in the composition", where, ordinal.value())));
      return std::move(ordinal).value();
    }

    // Pair targets of one (sub)lattice as a sparse map; missing entries mean target 0.
    //   no input -> empty map (every target 0)
    //   number   -> that target for every shell and every species pair xi <= eta
    //   list     -> listed entries; an entry without shell applies to every shell,
    //               an entry with shell overrides it for that shell
    // Keys are canonical: species sorted by atomic number (xi <= eta), real shell numbers.
    // shell_radii[0] = 0 is the site itself, shell 1 is the nearest-neighbour shell.
    // where: prefix for error messages ("" or "sublattice i: ")
    template <class T> result_t<pair_sro_t<T>> validate_pair_targets(
        const std::optional<std::variant<T, std::vector<pair_target_input<T>>>>& input,
        const std::vector<specie_t>& present, const std::vector<T>& shell_radii,
        std::string_view where) {
      using result = result_t<pair_sro_t<T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(pair_targets_error(code, std::format("{}{}", where, message)));
      };

      if (shell_radii.size() < 2)
        return fail(error_code::invalid_size, "no coordination shells available");
      const auto num_shells = static_cast<shell_t>(shell_radii.size() - 1);

      pair_sro_t<T> out;
      if (!input) return out;

      // one number for everything
      if (const T* alpha = std::get_if<T>(&*input)) {
        if (!is_reachable_alpha(*alpha))
          return fail(std::format("target {} must be finite and <= 1", *alpha));
        std::vector<specie_t> species = present;
        std::ranges::sort(species);
        for (shell_t shell = 1; shell <= num_shells; ++shell)
          for (std::size_t i = 0; i < species.size(); ++i)
            for (std::size_t j = i; j < species.size(); ++j)
              out.emplace(core::pair_key{.shell = shell, .species = {species[i], species[j]}},
                          *alpha);
        return out;
      }

      const auto& entries = std::get<std::vector<pair_target_input<T>>>(*input);
      if (entries.empty()) return fail("must not be empty; omit the key for targets of 0");

      // entries without shell, applied to every shell at the end
      std::map<std::array<specie_t, 2>, T> for_all_shells;

      for (const auto& entry : entries) {
        const std::string name
            = std::format("{}-{}", to_string(entry.species[0]), to_string(entry.species[1]));

        auto xi = resolve_specie(entry.species[0], present, where);
        if (!xi) return std::unexpected(std::move(xi.error()));
        auto eta = resolve_specie(entry.species[1], present, where);
        if (!eta) return std::unexpected(std::move(eta.error()));

        std::array<specie_t, 2> species{*xi, *eta};
        std::ranges::sort(species);  // canonical: xi <= eta

        if (!is_reachable_alpha(entry.alpha))
          return fail(std::format("target of {} must be finite and <= 1", name));

        if (!entry.shell) {
          if (!for_all_shells.emplace(species, entry.alpha).second)
            return fail(std::format("{} without shell is listed more than once", name));
          continue;
        }

        const shell_t shell = *entry.shell;
        if (shell == 0) return fail("shell 0 is the site itself, the first shell is 1");
        if (shell > num_shells)
          return fail(
              std::format("shell {} does not exist, there are {} shells", shell, num_shells));
        if (!out.emplace(core::pair_key{.shell = shell, .species = species}, entry.alpha).second)
          return fail(std::format("{} in shell {} is listed more than once", name, shell));
      }

      // entries without shell: every shell, unless an entry with that shell exists
      for (const auto& [species, alpha] : for_all_shells)
        for (shell_t shell = 1; shell <= num_shells; ++shell)
          out.try_emplace(core::pair_key{.shell = shell, .species = species}, alpha);

      return out;
    }

  }  // namespace detail

  // present: distinct atomic numbers of the structure (interact) or of each sublattice (split)
  template <class T, sublattice_mode Mode>
  result_t<pair_targets_t<T, Mode>> validate_pair_targets_input(
      const pair_targets_input<T, Mode>& input,
      const sublattice_mode_nesting<std::vector<specie_t>, Mode>& present,
      const sublattice_mode_nesting<std::vector<T>, Mode>& shell_radii) {
    if constexpr (Mode == sublattice_mode::interact) {
      // key missing or null -> every target 0
      return detail::validate_pair_targets<T>(input ? *input : std::nullopt, present, shell_radii,
                                              "");
    } else {
      // split: one entry per sublattice; key missing -> targets 0 everywhere,
      // entry null -> targets 0 for that sublattice
      const std::size_t num_sublattices = shell_radii.size();
      if (present.size() != num_sublattices)
        return std::unexpected(detail::pair_targets_error(std::format(
            "{} species lists, but {} shell radii lists", present.size(), num_sublattices)));
      if (input && input->size() != num_sublattices)
        return std::unexpected(detail::pair_targets_error(std::format(
            "{} entries given, but there are {} sublattices", input->size(), num_sublattices)));

      pair_targets_t<T, Mode> out;
      out.reserve(num_sublattices);
      for (std::size_t i = 0; i < num_sublattices; ++i) {
        auto targets
            = detail::validate_pair_targets<T>(input ? (*input)[i] : std::nullopt, present[i],
                                               shell_radii[i], std::format("sublattice {}: ", i));
        if (!targets) return std::unexpected(std::move(targets.error()));
        out.push_back(std::move(*targets));
      }
      return out;
    }
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_PAIR_TARGETS_IMPL_HPP
