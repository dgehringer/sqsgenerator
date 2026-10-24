//
// Created by Dominik Gehringer on 18.11.24.
//

#ifndef SQSGEN_CONFIGURATION_STRUCTURE_H
#define SQSGEN_CONFIGURATION_STRUCTURE_H

#include <cstddef>
#include <expected>
#include <optional>
#include <ranges>
#include <variant>
#include <vector>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T, sublattice_mode Mode> using structure_t
      = sublattice_mode_nesting<core::structure<T>, Mode>;

  template <class T, core::array_mode AMode> struct structure_config_dto {
    core::tensor_representation_t<T, 2, AMode> lattice;
    core::tensor_representation_t<T, 2, AMode> coords;
    configuration_t species;
    std::optional<std::array<int, 3>> supercell;
  };

  template <class T> class structure_config {
  public:
    lattice_t<T> lattice;
    coords_t<T> coords;
    configuration_t species;
    std::optional<std::array<int, 3>> supercell;

    [[nodiscard]] core::structure<T> structure() const;

    template <sublattice_mode Mode>
    structure_t<T, Mode> for_mode(const std::vector<sublattice>& composition) {
      if constexpr (Mode == sublattice_mode::interact) {
        return structure();
      } else if constexpr (Mode == sublattice_mode::split) {
        return structure().apply_composition_and_decompose(composition);
      } else
        static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split,
                      "unhandled sublattice_mode");
    }
  };

  template <class T> struct stucture_definition_input {
    core::nested_t<T, 2> lattice;
    core::nested_t<T, 2> coords;
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
