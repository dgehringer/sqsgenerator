//
// Created by Dominik Gehringer on 18.11.24.
//

#ifndef SQSGEN_IO_CONFIG_STRUCTURE_H
#define SQSGEN_IO_CONFIG_STRUCTURE_H

#include "sqsgen/types.h"
#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/parsing.h"


namespace sqsgen::io {
  namespace config {

    template <string_literal key>
    parse_result<configuration_t> validate_ordinals(std::vector<int>&& ordinals, auto num_sites) {
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

    template <string_literal key>
    parse_result<configuration_t> validate_symbols(std::vector<std::string>&& symbols,
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

    template <string_literal key, class Document>
    parse_result<configuration_t> parse_species(Document const& doc, auto num_sites) {
      return get_either<key, std::vector<int>, std::vector<std::string>>(doc)
          .template collapse<configuration_t>(
              [=](std::vector<int>&& ordinals) {
                return validate_ordinals<key>(std::forward<std::vector<int>>(ordinals), num_sites);
              },
              [=](std::vector<std::string>&& symbols) {
                return validate_symbols<key>(std::forward<std::vector<std::string>>(symbols),
                                             num_sites);
              });
    }

    template <string_literal key, class Document>
    parse_result<std::array<int, 3>> parse_supercell(Document const& doc) {
      using result_t = parse_result<std::array<int, 3>>;
      return fmap(
                 [&](auto&& cell) {
                   return cell.and_then([&](auto&& supercell) -> result_t {
                     for (auto amount : supercell)
                       if (amount < 0)
                         return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
                             "A supercell replication factor must be positive");
                     return result_t{supercell};
                   });
                 },
                 get_either_optional<key, std::array<int, 3>>(doc))
          .value_or(result_t{std::array{1, 1, 1}});
    }

    template <string_literal key, class T, class Document>
    parse_result<structure_config<T>> parse_structure_config(Document const& document) {
      if (!accessor<Document>::contains(document, key.data))
        return parse_error::from_msg<key, CODE_NOT_FOUND>("You need to specify a structure");
      const auto doc = accessor<Document>::get(document, key.data);
      return get_as<"lattice", lattice_t<T>>(doc)
          .combine(get_as<"coords", coords_t<T>>(doc))
          .combine(parse_supercell<"supercell">(doc))
          .and_then([&](auto&& data) {
            auto [lattice, coords, supercell] = data;
            return parse_species<"species">(doc, coords.rows())
                .and_then([&](auto&& species) -> parse_result<structure_config<T>> {
                  return structure_config<T>{std::move(lattice), std::move(coords), species,
                                             std::move(supercell)};
                });
          });
    }
  };  // namespace config

}  // namespace sqsgen::io
#endif  // SQSGEN_IO_CONFIG_STRUCTURE_H
