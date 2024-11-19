//
// Created by Dominik Gehringer on 18.11.24.
//

#ifndef SQSGEN_IO_PARSER_H
#define SQSGEN_IO_PARSER_H

#include <optional>

#include "sqsgen/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/io/json.h"
#include "sqsgen/types.h"

namespace sqsgen::io::parser {

  template <core::helpers::string_literal key>
  parse_result_t<configuration_t> parse_species(nlohmann::json const& j, auto num_sites) {
    auto species = get_either<key, std::vector<int>, std::vector<std::string>>(j);
    if (holds_error(species)) return std::get<parse_error>(species);
    configuration_t configuration;
    if (std::holds_alternative<std::vector<int>>(species)) {
      for (auto ordinal : std::get<std::vector<int>>(species)) {
        if (0 <= ordinal && ordinal < core::KNOWN_ELEMENTS.size()) {
          configuration.push_back(ordinal);
        } else
          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
              std::format("An atomic element with ordinal number {} is not known to me", ordinal));
      }
    }
    if (std::holds_alternative<std::vector<std::string>>(species)) {
      for (const auto& element : std::get<std::vector<std::string>>(species)) {
        if (core::SYMBOL_MAP.contains(element)) {
          configuration.push_back(core::atom::from_symbol(element).Z);
        } else
          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
              std::format("An atomic element with name {} is not known to me", element));
      }
    }
    if (configuration.size() != num_sites)
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
          std::format("Number of coordinates ({}) does not match number of species {}",
                      configuration.size(), num_sites));
    return configuration;
  }

  template <core::helpers::string_literal key>
  parse_result_t<std::optional<std::vector<int>>> parse_which(nlohmann::json const& j,
                                                              configuration_t const& conf) {
    auto which_data = get_either_optional<key, std::vector<int>, std::vector<std::string>>(j);
    if (which_data.has_value()) {
      auto which_value = which_data.value();
      if (holds_error(which_value)) return std::get<parse_error>(which_value);
      if (std::holds_alternative<std::vector<int>>(which_value)) {
        auto indices = std::get<std::vector<int>>(which_value);
        for (auto index : indices)
          if (!(0 <= index && index < conf.size()))
            return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(std::format(
                "Invalid index {}, the specified structure contains {} sites", index, conf.size()));
        if (indices.empty())
          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>("You cannot specify an empty slice");
        return std::make_optional(indices);
      }
      if (std::holds_alternative<std::vector<std::string>>(which_value)) {
        auto distinct_species = core::count_species(conf);
        std::set<specie_t> unique_species;
        for (auto species : std::get<std::vector<std::string>>(which_value)) {
          if (!core::SYMBOL_MAP.contains(species))
            return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
                std::format("An atomic element with name {} is not known to me", species));
          auto ordinal = core::atom::from_symbol(species).Z;
          if (!distinct_species.contains(ordinal))
            return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(std::format(
                "The specified structure does not contain a site with species {}", species));
          unique_species.insert(ordinal);
        }
        std::vector<int> indices;
        core::helpers::for_each(
            [&](auto i) {
              if (unique_species.contains(conf.at(i))) return indices.push_back(i);
            },
            conf.size());
        return std::make_optional(indices);
      }
    }
      return std::nullopt;
  }

  template <core::helpers::string_literal key>
  parse_result_t<std::optional<std::array<int, 3>>> parse_supercell(nlohmann::json const& j) {
    auto supercell_data = get_optional<key, std::array<int, 3>>(j);
    if (supercell_data.has_value()) {
      auto supercell_value = supercell_data.value();
      if (holds_error(supercell_value)) return std::get<parse_error>(supercell_value);
      for (auto amount : std::get<std::array<int, 3>>(supercell_value))
        if (amount < 0)
          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
              "A supercell replication factor must be positive");
      return std::make_optional(std::get<std::array<int, 3>>(supercell_value));
    }
    return std::nullopt;
  }

  template <class T> class structure_configuration {
  public:
    lattice_t<T> lattice;
    coords_t<T> coords;
    configuration_t species;
    std::optional<std::array<int, 3>> supercell = std::nullopt;
    std::optional<std::vector<int>> which = std::nullopt;

    core::structure<T> structure(bool supercell = true, bool which = true) {
      core::structure<T> structure(lattice, coords, species);
      if (supercell) {
        auto [a, b, c] = this->supercell.value_or(std::array{1, 1, 1});
        structure = structure.supercell(a, b, c);
      }
      if (which) {
        auto slice = this->which.value_or(std::vector<int>{});
        if (!slice.empty()) structure = structure.sliced(slice);
      }
      return structure;
    }

    static parse_result_t<structure_configuration> from_json(const nlohmann::json& j) {
      auto structure_data = combine<lattice_t<T>, coords_t<T>>(get_as<"lattice", lattice_t<T>>(j),
                                                               get_as<"coords", coords_t<T>>(j));
      if (holds_error(structure_data)) return std::get<parse_error>(structure_data);
      auto [lattice, coords] = std::get<1>(structure_data);
      auto configuration_data = parse_species<"species">(j, coords.rows());
      if (holds_error(configuration_data)) return std::get<parse_error>(configuration_data);
      auto configuration = std::get<1>(configuration_data);
      auto which_data = parse_which<"which">(j, configuration);
      if (holds_error(which_data)) return std::get<parse_error>(which_data);
      auto which = std::get<1>(which_data);

      auto supercell_data = parse_supercell<"supercell">(j);
      if (holds_error(supercell_data)) return std::get<parse_error>(supercell_data);
      auto supercell = std::get<1>(supercell_data);
      return structure_configuration{lattice, coords, configuration, supercell, which};
    }
  };
}  // namespace sqsgen::io::parser
#endif  // SQSGEN_IO_PARSER_H
