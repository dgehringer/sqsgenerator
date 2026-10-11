

#include "sqsgen/configuration/structure.h"

#include <array>
#include <expected>
#include <string>
#include <variant>

#include "sqsgen/configuration/common.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace helpers = sqsgen::core::helpers;

  namespace core = sqsgen::core;
  namespace io = sqsgen::io;

  configuration_error forward_error(const std::string& key, io::parser_error err) {
    return {key, err.code, err.msg};
  }

  result_t<configuration_t> validate_ordinals(std::vector<int> const& ordinals, auto num_sites) {
    if (ordinals.size() != num_sites)
      return std::unexpected(
          "structure.species", error_code::invalid_size,
          helpers::format_string("Number of coordinates (%i) does not match number of species %i",
                                 num_sites, ordinals.size()));

    configuration_t conf;

    for (auto o : ordinals) {
      if (0 <= o && o < core::KNOWN_ELEMENTS.size())
        conf.push_back(o);
      else
        return std::unexpected(configuration_error{
            "structure.species", error_code::out_of_range,
            helpers::format_string("An atomic element with ordinal number %u is not known to me",
                                   o)});
    }
    return conf;
  }

  result_t<configuration_t> validate_symbols(std::vector<std::string>&& symbols, auto num_sites) {
    if (symbols.size() != num_sites)
      return std::unexpected(
          "structure.species", error_code::invalid_size,
          helpers::format_string(
              "Number of coordinates (%u) does not match number of species %u specified", num_sites,
              symbols.size()));
    configuration_t conf;
    for (const auto& element : symbols)
      if (core::SYMBOL_MAP.contains(element))
        conf.push_back(core::atom::from_symbol(element).Z);
      else
        return std::unexpected(configuration_error{
            "structure.species", error_code::out_of_range,
            helpers::format_string("An atomic element with name \"%s\" is not known to me",
                                   element)});
    return conf;
  }

  result_t<configuration_t> validate_species(
      std::variant<std::vector<std::string>, std::vector<int>> const& species, auto num_sites) {
    return std::visit(helpers::overloaded{[&](std::vector<std::string> const& species) {
                                            return validate_symbols(species, num_sites);
                                          },
                                          [&](std::vector<int> const& species) {
                                            return validate_ordinals(species, num_sites);
                                          }},
                      species);
  }

  result_t<std::array<int, 3>> validate_supercell(std::array<int, 3> const& supercell) {
    for (auto amount : supercell)
      if (amount < 0)
        return std::unexpected(
            configuration_error{"structure.supercell", error_code::out_of_range,
                                "A supercell replication factor must be positive"});
    return supercell;
  }

  template <class T> result_t<lattice_t<T>> validate_lattice(core::nested_t<T, 2> const& matrix) {
    auto lattice = core::tensor_from<core::nested_tensor<T, 2>>(matrix);
    if (!lattice)
      return lattice.transform_error([](auto err) {
        return configuration_error{"structure.lattice", error_code::bad_argument, err.message};
      });
    if (lattice->rows() == 3 && lattice->cols() == 3)
      return *lattice;
    else
      return std::unexpected(configuration_error{"structure.lattice", error_code::invalid_size,
                                                 "A lattice must be a 3x3 matrix"});
  };

  template <class T> result_t<coords_t<T>> validate_coords(core::nested_t<T, 2> const& matrix) {
    auto coords = core::tensor_from<core::nested_tensor<T, 2>>(matrix);
    if (!coords)
      return coords.transform_error([](auto err) {
        return configuration_error{"structure.coords", error_code::bad_argument, err.message};
      });
    if (coords->rows() > 1)
      return *coords;
    else
      return std::unexpected(configuration_error{"structure.coords", error_code::invalid_size,
                                                 "A structure must contain at least one site"});
  };

  template <class T> result_t<structure_config<T>> validate_structure_definition(
      stucture_definition_input<T> const& input) {
    auto coords = validate_coords(input.coords);
    if (!coords) return std::unexpected(std::move(coords).error());
    auto species = validate_species(input.species, coords->rows());
    if (!species) return std::unexpected(std::move(species).error());
    auto lattice = validate_lattice(input.lattice);
    if (!lattice) return std::unexpected(std::move(lattice).error());
    auto supercell = validate_supercell(input.supercell);
    if (!supercell) return std::unexpected(std::move(supercell).error());
    return structure_config<T>{*lattice, *coords, *species, *supercell};
  }

  result_t<io::structure_format> structure_format_from_path(std::string const& path) {
    if (path.ends_with(".pymatgen.json")) return io::structure_format::pymatgen;

    if (path.ends_with(".sqs.json")) return io::structure_format::sqsgen;

    if (path.ends_with(".vasp") || path.ends_with(".poscar")) return io::structure_format::poscar;

    return std::unexpected(configuration_error{
        "structure.format", error_code::bad_argument,
        helpers::format_string("Unsupported file extension \"%s\". Currently only .pymatgen.json, "
                               ".sqs.json, .vasp and .poscar are supported",
                               path)});
  }

  result_t<std::string> read_file(std::string const& path) {
    if (!std::filesystem::exists(path))
      return std::unexpected(
          configuration_error{"structure.path", error_code::bad_argument,
                              helpers::format_string("The file \"%s\" does not exist", path)});
    std::ifstream ifs(path);
    if (!ifs)
      return std::unexpected(
          configuration_error{"structure.path", error_code::bad_argument,
                              helpers::format_string("Could not open file: %s", path)});
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }

  template <class T>
  result_t<core::structure<T>> parse_file(std::string const& path, io::structure_format format) {
    auto content = read_file(path);
    if (!content) return std::unexpected(std::move(content).error());
    if (format == io::structure_format::pymatgen)
      return content.and_then([](auto&& data) {
        auto structure = io::structure_adapter<T, io::structure_format::pymatgen>::read(
            std::move(data), io::wire_format::json);
        if (!structure)
          return std::unexpected(forward_error("structure.path", std::move(structure).error()));
        return *structure;
      });
    if (format == io::structure_format::sqsgen)
      return content.and_then([](auto&& data) {
        auto structure = io::structure_adapter<T, io::structure_format::sqsgen>::read(
            std::move(data), io::wire_format::json);
        if (!structure)
          return std::unexpected(forward_error("structure.path", std::move(structure).error()));
        return *structure;
      });

    if (format == io::structure_format::poscar)
      return content.and_then([](auto&& data) {
        auto structure
            = io::structure_adapter<T, io::structure_format::poscar>::read(std::move(data));
        if (!structure)
          return std::unexpected(forward_error("structure.path", std::move(structure).error()));
        return *structure;
      });
  }

  template <class T>
  result_t<structure_config<T>> validate_structure_file(const stucture_file_input& input) {
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

    return structure_config<T>{structure->lattice, structure->frac_coords, structure->species,
                               *supercell};
  }

  template <class T>
  std::expected<structure_config<T>, configuration_error> validate_structure_input(
      const structure_input<T>& input) {
    return std::visit(
        helpers::overloaded{
            [&](const stucture_definition_input<T>& input) {
              return validate_structure_definition(input);
            },
            [&](const stucture_file_input& input) { return validate_structure_file<T>(input); },
        },
        input);
  }

  template <> std::expected<structure_config<float>, configuration_error> validate_structure_input(
      const structure_input<float>& input);
  template <> std::expected<structure_config<double>, configuration_error> validate_structure_input(
      const structure_input<double>& input);

};  // namespace sqsgen::configuration
