//
// Created by Dominik Gehringer on 18.11.24.
//

#ifndef SQSGEN_CONFIGURATION_STRUCTURE_H
#define SQSGEN_CONFIGURATION_STRUCTURE_H

#include <cstddef>
#include <expected>
#include <fstream>
#include <variant>
#include <vector>

#include "sqsgen/configuration/common.h"
#include "sqsgen/core/atom.h"
#include "sqsgen/core/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/parsing.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/log.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T> class structure_config {
  public:
    lattice_t<T> lattice;
    coords_t<T> coords;
    configuration_t species;
    std::array<int, 3> supercell{1, 1, 1};
  };

  template <class T> struct stucture_definition_input {
    nested_t<T, 2> lattice;
    nested_t<T, 2> coords;
    bool frac = true;
    std::variant<std::vector<std::string>, std::vector<int>> species;
    std::array<int, 3> supercell{1, 1, 1};
  };

  struct stucture_file_input {
    std::optional<structure_format> format;
    std::string path;
    std::array<int, 3> supercell{1, 1, 1};
  };

  template <class T> using structure_input
      = std::variant<stucture_definition_input<T>, stucture_file_input>;

  template <class T> struct configuration_parser
      : configuration_parse_base<"structure", structure_input<T>, structure_config<T>> {
    std::expected<structure_config<T>, configuration_error> parse(structure_input<T> const& input);
  };

};  // namespace sqsgen::configuration

namespace sqsgen::io {
  namespace config {

    using namespace sqsgen::core;

    template <string_literal key>
    inline parse_result<std::string> read_file(std::string const& filename) {
      if (!std::filesystem::exists(filename))
        return parse_error::from_msg<key, CODE_NOT_FOUND>(
            format_string("The file \"%s\" does not exist", filename));
      std::ifstream ifs(filename);
      if (!ifs)
        parse_error::from_msg<key, CODE_BAD_ARGUMENT>(
            format_string("Could not open file: %s", filename));
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
          format_string("Unsupported file extension \"%s\". Currently only .pymatgen.json, "
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
#endif  // SQSGEN_CONFIGURATION_STRUCTURE_H
