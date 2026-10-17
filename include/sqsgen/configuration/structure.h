//
// Created by Dominik Gehringer on 18.11.24.
//

#ifndef SQSGEN_CONFIGURATION_STRUCTURE_H
#define SQSGEN_CONFIGURATION_STRUCTURE_H

#include <cstddef>
#include <expected>
#include <optional>
#include <variant>
#include <vector>

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
    core::nested_t<T, 2> lattice;
    core::nested_t<T, 2> coords;
    bool frac = true;
    std::variant<std::vector<std::string>, std::vector<int>> species;
    std::optional<std::array<int, 3>> supercell;
  };

  struct structure_file_input {
    std::optional<sqsgen::io::structure_format> format;
    std::string path;
    std::optional<std::array<int, 3>> supercell;
  };

  template <class T> using structure_input
      = std::variant<stucture_definition_input<T>, structure_file_input>;

  template <class T>
  std::expected<structure_config<T>, configuration_error> validate_structure_input(
      const structure_input<T>&);

};  // namespace sqsgen::configuration

#endif  // SQSGEN_CONFIGURATION_STRUCTURE_H
