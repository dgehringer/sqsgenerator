//
// Created by Dominik Gehringer on 18.11.24.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_STRUCTURE_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_STRUCTURE_IMPL_HPP

#include <array>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <variant>

#include "sqsgen/configuration/structure.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace helpers = sqsgen::core::helpers;
  namespace io = sqsgen::io;

  inline configuration_error forward_error(const std::string& key, io::parser_error err) {
    return {key, err.code, err.msg};
  }

  result_t<configuration_t> validate_ordinals(const std::vector<int>& ordinals, auto num_sites) {
    if (ordinals.size() != num_sites)
      return std::unexpected(configuration_error{
          "structure.species", error_code::invalid_size,
          std::format("Number of coordinates ({}) does not match number of species {}", num_sites,
                      ordinals.size())});

    configuration_t conf;

    for (auto o : ordinals) {
      if (0 <= o && o < static_cast<int>(core::KNOWN_ELEMENTS.size()))
        conf.push_back(o);
      else
        return std::unexpected(configuration_error{
            "structure.species", error_code::out_of_range,
            std::format("An atomic element with ordinal number {} is not known to me", o)});
    }
    return conf;
  }

  result_t<configuration_t> validate_symbols(const std::vector<std::string>& symbols,
                                             auto num_sites) {
    if (symbols.size() != num_sites)
      return std::unexpected(configuration_error{
          "structure.species", error_code::invalid_size,
          std::format("Number of coordinates ({}) does not match number of species {} specified",
                      num_sites, symbols.size())});
    configuration_t conf;
    for (const auto& element : symbols)
      if (core::SYMBOL_MAP.contains(element))
        conf.push_back(core::atom::from_symbol(element).Z);
      else
        return std::unexpected(configuration_error{
            "structure.species", error_code::out_of_range,
            std::format("An atomic element with name \"{}\" is not known to me", element)});
    return conf;
  }

  result_t<configuration_t> validate_species(
      const std::variant<std::vector<std::string>, std::vector<int>>& species, auto num_sites) {
    return std::visit(helpers::overloaded{[&](const std::vector<std::string>& species) {
                                            return validate_symbols(species, num_sites);
                                          },
                                          [&](const std::vector<int>& species) {
                                            return validate_ordinals(species, num_sites);
                                          }},
                      species);
  }

  inline result_t<std::optional<std::array<int, 3>>> validate_supercell(
      const std::optional<std::array<int, 3>>& supercell) {
    if (supercell.has_value())
      for (auto amount : supercell.value())
        if (amount <= 0)
          return std::unexpected(
              configuration_error{"structure.supercell", error_code::out_of_range,
                                  "A supercell replication factor must be positive"});
    return supercell;
  }

  template <class T> result_t<lattice_t<T>> validate_lattice(const core::nested_t<T, 2>& matrix) {
    auto lattice = core::lattice_from<T>(matrix);
    if (!lattice)
      return lattice.transform_error([](auto err) {
        return configuration_error{"structure.lattice", error_code::bad_argument, err.message};
      });
    if (lattice->rows() == 3 && lattice->cols() == 3)
      return std::move(lattice).value();
    else
      return std::unexpected(configuration_error{"structure.lattice", error_code::invalid_size,
                                                 "A lattice must be a 3x3 matrix"});
  }

  template <class T> result_t<coords_t<T>> validate_coords(const core::nested_t<T, 2>& matrix) {
    auto coords = core::coords_from<T>(matrix);
    if (!coords)
      return coords.transform_error([](auto err) {
        return configuration_error{"structure.coords", error_code::bad_argument, err.message};
      });
    if (coords->rows() >= 1)
      return std::move(coords).value();
    else
      return std::unexpected(configuration_error{"structure.coords", error_code::invalid_size,
                                                 "A structure must contain at least one site"});
  }

  template <class T> result_t<structure_config<T>> validate_structure_definition(
      const stucture_definition_input<T>& input) {
    auto coords = validate_coords<T>(input.coords);
    if (!coords) return std::unexpected(std::move(coords).error());
    auto species = validate_species(input.species, coords->rows());
    if (!species) return std::unexpected(std::move(species).error());
    auto lattice = validate_lattice<T>(input.lattice);
    if (!lattice) return std::unexpected(std::move(lattice).error());
    auto supercell = validate_supercell(input.supercell);
    if (!supercell) return std::unexpected(std::move(supercell).error());
    return structure_config<T>{
        core::structure<T>(std::move(lattice).value(), std::move(coords).value(),
                           std::move(species).value()),
        std::move(supercell).value()};
  }

  inline result_t<io::structure_format> structure_format_from_path(const std::string& path) {
    if (path.ends_with(".pymatgen.json")) return io::structure_format::pymatgen;

    if (path.ends_with(".sqs.json")) return io::structure_format::sqsgen;

    if (path.ends_with(".vasp") || path.ends_with(".poscar")) return io::structure_format::poscar;

    return std::unexpected(configuration_error{
        "structure.format", error_code::bad_argument,
        std::format("Unsupported file extension \"{}\". Currently only .pymatgen.json, "
                    ".sqs.json, .vasp and .poscar are supported",
                    path)});
  }

  inline result_t<std::string> read_file(const std::string& path) {
    if (!std::filesystem::exists(path))
      return std::unexpected(
          configuration_error{"structure.path", error_code::bad_argument,
                              std::format("The file \"{}\" does not exist", path)});
    std::ifstream ifs(path);
    if (!ifs)
      return std::unexpected(configuration_error{"structure.path", error_code::bad_argument,
                                                 std::format("Could not open file: {}", path)});
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }

  template <class T>
  result_t<core::structure<T>> parse_file(const std::string& path, io::structure_format format) {
    auto content = read_file(path);
    if (!content) return std::unexpected(std::move(content).error());
    if (format == io::structure_format::pymatgen)
      return content.and_then([](auto&& data) -> result_t<core::structure<T>> {
        auto structure
            = io::structure_adapter<T, io::structure_format::pymatgen>::read(std::move(data));
        if (!structure)
          return std::unexpected(forward_error("structure.path", std::move(structure).error()));
        return std::move(structure).value();
      });
    if (format == io::structure_format::sqsgen)
      return content.and_then([](auto&& data) -> result_t<core::structure<T>> {
        auto structure = io::structure_adapter<T, io::structure_format::sqsgen>::read(
            std::move(data), io::wire_format::json);
        if (!structure)
          return std::unexpected(forward_error("structure.path", std::move(structure).error()));
        return std::move(structure).value();
      });

    if (format == io::structure_format::poscar)
      return content.and_then([](auto&& data) -> result_t<core::structure<T>> {
        auto structure
            = io::structure_adapter<T, io::structure_format::poscar>::read(std::move(data));
        if (!structure)
          return std::unexpected(forward_error("structure.path", std::move(structure).error()));
        return std::move(structure).value();
      });

    return std::unexpected(configuration_error{"structure.format", error_code::bad_argument,
                                               "The structure format specified has no parser "
                                               "implemented and is meant for write-only use."});
  }

  template <class T>
  result_t<structure_config<T>> validate_structure_file(const structure_file_input& input) {
    io::structure_format format = io::structure_format::sqsgen;
    if (!input.format) {
      if (auto format_from_path = structure_format_from_path(input.path); format_from_path)
        format = *format_from_path;
      else
        return std::unexpected(format_from_path.error());
    } else if (auto parsed_format = input.format.value();
               parsed_format == io::structure_format::sqsgen
               || parsed_format == io::structure_format::pymatgen
               || parsed_format == io::structure_format::poscar) {
      format = parsed_format;

    } else
      return std::unexpected(configuration_error{
          "structure.format", error_code::bad_argument,
          "Only json_ase, json_pymatgen, json_sqsgen and poscar are allowed for "
          "reading. cif and pdb are write only"});

    auto structure = parse_file<T>(input.path, format);
    if (!structure) return std::unexpected(std::move(structure).error());

    auto supercell = validate_supercell(input.supercell);
    if (!supercell) return std::unexpected(std::move(supercell).error());

    return structure_config<T>{core::structure<T>(std::move(structure).value()),
                               std::move(supercell).value()};
  }

  template <class T>
  result_t<structure_config<T>> validate_structure_input(const structure_input<T>& input) {
    return std::visit(
        helpers::overloaded{
            [&](const stucture_definition_input<T>& input) {
              return validate_structure_definition<T>(input);
            },
            [&](const structure_file_input& input) { return validate_structure_file<T>(input); },
        },
        input);
  }

  template <class T> core::structure<T> structure_config<T>::make_structure() const {
    if (supercell.has_value()) {
      auto [a, b, c] = supercell.value();
      return structure.supercell(a, b, c);
    }
    return structure;
  }

  template <class T> template <core::array_mode AMode>
  structure_config_dto<T, AMode> structure_config<T>::to_dto() const {
    return structure_config_dto<T, AMode>{.structure = structure.template to_dto<AMode>(),
                                          .supercell = supercell};
  }

  template <class T> template <core::array_mode AMode>
  io::result_t<structure_config<T>> structure_config<T>::from_dto(
      const structure_config_dto<T, AMode>& data) {
    auto structure = core::structure<T>::template from_dto<AMode>(data.structure);
    if (!structure) return std::unexpected(std::move(structure).error());
    return structure_config<T>{
        .structure = std::move(structure).value(),
        .supercell = data.supercell,
    };
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_STRUCTURE_IMPL_HPP
