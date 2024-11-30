//
// Created by Dominik Gehringer on 18.11.24.
//

#ifndef SQSGEN_IO_CONFIG_STRUCTURE_H
#define SQSGEN_IO_CONFIG_STRUCTURE_H

#include <optional>

#include "sqsgen/core/helpers.h"
#include "sqsgen/io/json.h"
#include "sqsgen/types.h"

namespace sqsgen::io::config {

  template <class T> class structure_config {
  public:
    static constexpr auto DEFAULT_SUPERCELL = std::array{1, 1, 1};
    lattice_t<T> lattice;
    coords_t<T> coords;
    configuration_t species;
    std::optional<std::array<int, 3>> supercell = std::nullopt;

    core::structure<T> structure(bool supercell = true) {
      core::structure<T> structure(lattice, coords, species);
      if (supercell) {
        auto [a, b, c] = this->supercell.value_or(DEFAULT_SUPERCELL);
        structure = structure.supercell(a, b, c);
      }
      return structure;
    }
  };

  template <core::helpers::string_literal key>
  parse_result_t<configuration_t> validate_ordinals(std::vector<int>&& ordinals, auto num_sites) {
    if (ordinals.size() != num_sites)
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
          std::format("Number of coordinates ({}) does not match number of species {}", num_sites,
                      ordinals.size()));
    configuration_t conf;
    for (auto o : ordinals) {
      if (0 <= o && o < core::KNOWN_ELEMENTS.size())
        conf.push_back(o);
      else
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
            std::format("An atomic element with ordinal number {} is not known to me", o));
    }
    return conf;
  }

  template <core::helpers::string_literal key>
  parse_result_t<configuration_t> validate_symbols(std::vector<std::string>&& symbols,
                                                   auto num_sites) {
    if (symbols.size() != num_sites)
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
          std::format("Number of coordinates ({}) does not match number of species {}", num_sites,
                      symbols.size()));
    configuration_t conf;
    for (const auto& element : symbols)
      if (core::SYMBOL_MAP.contains(element))
        conf.push_back(core::atom::from_symbol(element).Z);
      else
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
            std::format("An atomic element with name {} is not known to me", element));

    return conf;
  }

  template <core::helpers::string_literal key>
  parse_result_t<configuration_t> parse_species(nlohmann::json const& j, auto num_sites) {
    using out_t = parse_result_t<configuration_t>;
    return validate<configuration_t>(
        get_either<key, std::vector<int>, std::vector<std::string>>(j),
        [=](std::vector<int>&& ordinals) -> out_t {
          return validate_ordinals<key>(std::forward<std::vector<int>>(ordinals), num_sites);
        },
        [=](std::vector<std::string>&& symbols) -> out_t {
          return validate_symbols<key>(std::forward<std::vector<std::string>>(symbols), num_sites);
        });
  }

  template <core::helpers::string_literal key>
  parse_result_t<std::optional<std::array<int, 3>>> parse_supercell(nlohmann::json const& j) {
    return validate(get_optional<key, std::array<int, 3>>(j),
                    [&](auto&& supercell) -> parse_result_t<std::array<int, 3>> {
                      for (auto amount : supercell)
                        if (amount < 0)
                          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
                              "A supercell replication factor must be positive");
                      return supercell;
                    });
  }

  template <class T> static parse_result_t<structure_config<T>> parse_structure_config(const nlohmann::json& j) {
    auto structure_data = combine<lattice_t<T>, coords_t<T>>(get_as<"lattice", lattice_t<T>>(j),
                                                             get_as<"coords", coords_t<T>>(j));
    if (holds_error(structure_data)) return std::get<parse_error>(structure_data);
    auto [lattice, coords] = get_result(structure_data);
    auto configuration_data = parse_species<"species">(j, coords.rows());
    if (holds_error(configuration_data)) return std::get<parse_error>(configuration_data);
    auto configuration = get_result(configuration_data);
    auto supercell_data = parse_supercell<"supercell">(j);
    if (holds_error(supercell_data)) return std::get<parse_error>(supercell_data);
    auto supercell = get_result(supercell_data);
    return structure_config{lattice, coords, configuration, supercell};
  }
};  // namespace sqsgen::io::config
// namespace sqsgen::io::config
#endif  // SQSGEN_IO_CONFIG_STRUCTURE_H
