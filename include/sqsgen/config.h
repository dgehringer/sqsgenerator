//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include <optional>

#include "io/json.h"

namespace sqsgen {

  template <class T> class structure_configuration {
  public:
    lattice_t<T> lattice;
    coords_t<T> coords;
    configuration_t species;
    std::optional<std::array<int, 3>> supercell = std::nullopt;
    std::optional<std::vector<std::size_t>> which = std::nullopt;

    core::structure<T> structure(bool supercell = true, bool which = true) {
      core::structure<T> structure(lattice, coords, species);
      if (supercell) {
        auto [a, b, c] = this->supercell.value_or(std::array{1, 1, 1});
        structure = structure.supercell(a, b, c);
      }
      if (which) {
        auto slice = this->which.value_or(std::vector<std::size_t>{});
        if (!slice.empty()) structure = structure.sliced(slice);
      }
      return structure;
    }

    static parse_result_t<structure_configuration> from_json(const nlohmann::json& j) {
      auto structure_data = combine<lattice_t<T>, coords_t<T>>(get_as<"lattice", lattice_t<T>>(j),
                                                               get_as<"coords", coords_t<T>>(j));
      if (holds_error(structure_data)) return std::get<parse_error>(structure_data);
      auto species = get_either<"species", std::vector<int>, std::vector<std::string>>(j);
      if (holds_error(species)) return std::get<parse_error>(species);
      auto [lattice, coords] = std::get<1>(structure_data);
      configuration_t configuration;

      if (std::holds_alternative<std::vector<int>>(species)) {
        for (auto ordinal : std::get<std::vector<int>>(species)) {
          if (0 <= ordinal && ordinal < core::KNOWN_ELEMENTS.size()) {
            configuration.push_back(ordinal);
          } else
            return parse_error::from_msg<"species", CODE_OUT_OF_RANGE>(std::format(
                "An atomic element with ordinal number {} is not known to me", ordinal));
        }
      }
      if (std::holds_alternative<std::vector<std::string>>(species)) {
        for (const auto& element : std::get<std::vector<std::string>>(species)) {
          if (core::SYMBOL_MAP.contains(element)) {
            configuration.push_back(core::atom::from_symbol(element).Z);
          } else
            return parse_error::from_msg<"species", CODE_OUT_OF_RANGE>(
                std::format("An atomic element with name {} is not known to me", element));
        }
      }
    }
  };

  template <class T> struct configuration {};

}  // namespace sqsgen

#endif  // SQSGEN_CONFIGURATION_H
