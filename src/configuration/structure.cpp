

#include "sqsgen/configuration/structure.h"

#include <array>
#include <expected>
#include <string>
#include <variant>

#include "sqsgen/configuration/common.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  result_t<configuration_t> validate_ordinals(std::vector<int> const& ordinals, auto num_sites) {
    static constexpr std::string key = "structure.species";
    if (ordinals.size() != num_sites)
      return std::unexpected(
          key, error_code::invalid_size,
          format_string("Number of coordinates (%i) does not match number of species %i", num_sites,
                        ordinals.size()));

    configuration_t conf;

    for (auto o : ordinals) {
      if (0 <= o && o < core::KNOWN_ELEMENTS.size())
        conf.push_back(o);
      else
        return std::unexpected(configuration_error{
            key, error_code::out_of_range,
            format_string("An atomic element with ordinal number %u is not known to me", o)});
    }
    return conf;
  }

  result_t<configuration_t> validate_symbols(std::vector<std::string>&& symbols, auto num_sites) {
    static constexpr std::string key = "structure.species";
    if (symbols.size() != num_sites)
      return std::unexpected(
          key, error_code::invalid_size,
          format_string("Number of coordinates (%u) does not match number of species %u specified",
                        num_sites, symbols.size()));
    configuration_t conf;
    for (const auto& element : symbols)
      if (core::SYMBOL_MAP.contains(element))
        conf.push_back(core::atom::from_symbol(element).Z);
      else
        return std::unexpected(configuration_error{
            key, error_code::out_of_range,
            format_string("An atomic element with name \"%s\" is not known to me", element)});
    return conf;
  }

  result_t<configuration_t> validate_species(
      std::variant<std::vector<std::string>, std::vector<int>> const& species, auto num_sites) {
    return std::visit(overloaded{[&](std::vector<std::string> const& species) {
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

  template <class T> result_t<lattice_t<T>> validate_lattice(nested_tensor<T, 2> const& matrix) {
    static constexpr std::string key = "structure.lattice";
    auto lattice = core::tensor_from<nested_tensor<T, 2>>(matrix);
    if (!lattice)
      return lattice.transform_error(
          [](auto err) { return configuration_error{key, error_code::bad_argument, err.message}; });
    if (lattice->rows() == 3 && lattice->cols() == 3)
      return *lattice;
    else
      return std::unexpected(
          configuration_error{key, error_code::invalid_size, "A lattice must be a 3x3 matrix"});
  };

  template <class T> result_t<coords_t<T>> validate_coords(nested_tensor<T, 2> const& matrix) {
    static constexpr std::string key = "structure.coords";
    auto coords = core::tensor_from<nested_tensor<T, 2>>(matrix);
    if (!coords)
      return coords.transform_error(
          [](auto err) { return configuration_error{key, error_code::bad_argument, err.message}; });
    if (coords->rows() > 1)
      return *coords;
    else
      return std::unexpected(configuration_error{key, error_code::invalid_size,
                                                 "A structure must contain at least one site"});
  };

  template <class T> result_t<structure_config<T>> validate_structure_definition(
      stucture_definition_input<T> const& input) {
    auto coords = validate_coords(input.coords);
    if (!coords) return std::unexpected(coords.error());
    auto species = validate_species(input.species, coords->rows());
    if (!species) return std::unexpected(species.error());
    auto lattice = validate_lattice(input.lattice);
    if (!lattice) return std::unexpected(lattice.error());
    auto supercell = validate_supercell(input.supercell);
    if (!supercell) return std::unexpected(supercell.error());
    return structure_config<T>{*lattice, *coords, *species, *supercell};
  }

  result_t<structure_format> structure_format_from_path(std::string const& path) {
    if (path.ends_with(".pymatgen.json")) return structure_format::json_pymatgen;

    if (path.ends_with(".sqs.json")) return structure_format::json_sqsgen;

    if (path.ends_with(".vasp") || path.ends_with(".poscar")) return structure_format::poscar;

    return std::unexpected(configuration_error{
        "structure.format", error_code::bad_argument,
        format_string("Unsupported file extension \"%s\". Currently only .pymatgen.json, "
                      ".sqs.json, .vasp and .poscar are supported",
                      path)});
  }

  result_t<std::string> read_file(std::string const& path) {
    static constexpr std::string key = "structure.path";
    if (!std::filesystem::exists(path))
      return std::unexpected(configuration_error{
          key, error_code::bad_argument, format_string("The file \"%s\" does not exist", path)});
    std::ifstream ifs(path);
    if (!ifs)
      return std::unexpected(configuration_error{key, error_code::bad_argument,
                                                 format_string("Could not open file: %s", path)});
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }

  template <class T>
  result_t<core::structure<T>> validate_path(std::string const& path, structure_format format) {
    if (format == structure_format::json_pymatgen)
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

    template <class T>
    result_t<structure_config<T>> validate_structure_file(stucture_file_input const& input) {
      structure_format format = structure_format::json_sqsgen;
      if (!input.format) {
        if (auto format_from_path = structure_format_from_path(input.path); format_from_path)
          format = *format_from_path;
        else
          return std::unexpected(format_from_path.error());
      } else if (auto parsed_format = input.format.value();
                 parsed_format == structure_format::json_ase || structure_format::json_pymatgen
                 || structure_format::poscar) {
        format = parsed_format;

      } else
        return std::unexpected(configuration_error{
            "structure.format", error_code::bad_argument,
            "Only json_ase, json_pymatgen, json_sqsgen and poscar are allowed for "
            "reading. cif and pdb are write only"});
    }

  };  // namespace sqsgen::configuration
