

#include "sqsgen/configuration/structure.h"

#include <Eigen/Core>
#include <array>
#include <cstddef>
#include <expected>
#include <string>
#include <variant>
#include <vector>

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

  template <class T> result_t<structure_config<T>> parse_structure_definition(
      stucture_definition_input<T> const& input) {}

};  // namespace sqsgen::configuration
