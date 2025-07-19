//
// Created by Dominik Gehringer on 18.11.24.
//

#ifndef SQSGEN_IO_CONFIG_STRUCTURE_H
#define SQSGEN_IO_CONFIG_STRUCTURE_H

#include <fstream>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/parsing.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {
  namespace config {

    using namespace sqsgen::core;

    template <string_literal key>
    parse_result<configuration_t> validate_ordinals(std::vector<int>&& ordinals, auto num_sites) {
      if (ordinals.size() != num_sites)
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
            fmt::format("Number of coordinates ({}) does not match number of species {}", num_sites,
                        ordinals.size()));
      configuration_t conf;
      for (auto o : ordinals) {
        if (0 <= o && o < core::KNOWN_ELEMENTS.size())
          conf.push_back(o);
        else
          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
              fmt::format("An atomic element with ordinal number {} is not known to me", o));
      }
      return conf;
    }

    template <string_literal key>
    parse_result<configuration_t> validate_symbols(std::vector<std::string>&& symbols,
                                                   auto num_sites) {
      if (symbols.size() != num_sites)
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
            fmt::format("Number of coordinates ({}) does not match number of species {}", num_sites,
                        symbols.size()));
      configuration_t conf;
      for (const auto& element : symbols)
        if (core::SYMBOL_MAP.contains(element))
          conf.push_back(core::atom::from_symbol(element).Z);
        else
          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
              fmt::format("An atomic element with name {} is not known to me", element));
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

    template <string_literal key>
    inline parse_result<std::string> read_file(std::string const& filename) {
      if (!std::filesystem::exists(filename))
        return parse_error::from_msg<key, CODE_NOT_FOUND>(
            fmt::format("The file {} does not exist", filename));
      std::ifstream ifs(filename);
      if (!ifs)
        parse_error::from_msg<key, CODE_BAD_ARGUMENT>(
            fmt::format("Could not open file: {}", filename));
      std::ostringstream oss;
      oss << ifs.rdbuf();
      return oss.str();
    }

    template <string_literal key, class T>
    parse_result<structure<T>> parse_structure_from_file(std::string const& path) {
      if (path.ends_with(".pymatgen.json"))
        return read_file<key>(path).and_then([&](auto&& data) {
          return io::structure_adapter<T, STRUCTURE_FORMAT_JSON_PYMATGEN>::from_json(
              std::forward<std::string>(data));
        });

      if (path.ends_with(".sqs.json"))
        return read_file<key>(path).and_then([&](auto&& data) {
          return io::structure_adapter<T, STRUCTURE_FORMAT_JSON_SQSGEN>::from_json(
              std::forward<std::string>(data));
        });

      if (path.ends_with(".vasp") || path.ends_with(".poscar"))
        return read_file<key>(path).and_then([&](auto&& data) {
          return io::structure_adapter<T, STRUCTURE_FORMAT_POSCAR>::from_string(
              std::forward<std::string>(data));
        });

      return parse_error::from_msg<key, CODE_BAD_ARGUMENT>(
          fmt::format("Unsupported file extension \"{}\". Currently only .pymatgen.json, "
                      ".sqs.json, .vasp and .poscar are supported",
                      path));
    }

    template <string_literal key, class T, class Document>
    parse_result<structure_config<T>> parse_structure_config(Document const& document) {
      if (!accessor<Document>::contains(document, key.data))
        return parse_error::from_msg<key, CODE_NOT_FOUND>("You need to specify a structure");
      const auto doc = accessor<Document>::get(document, key.data);
      if (accessor<Document>::contains(doc, "file"))
        return get_as<"file", std::string>(doc)
            .and_then(parse_structure_from_file<key, T>)
            .combine(parse_supercell<"supercell">(doc))
            .and_then([](auto&& structure_and_supercell) -> parse_result<structure_config<T>> {
              auto [structure, supercell] = structure_and_supercell;
              return structure_config<T>{structure.lattice, structure.frac_coords,
                                         structure.species, std::move(supercell)};
            });
      else
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
