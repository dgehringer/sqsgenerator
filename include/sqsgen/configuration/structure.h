//
// Created by Dominik Gehringer on 18.11.24.
//

#ifndef SQSGEN_CONFIGURATION_STRUCTURE_H
#define SQSGEN_CONFIGURATION_STRUCTURE_H

#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;

  template <class T, sublattice_mode Mode> using structure_t
      = sublattice_mode_nesting<core::structure<T>, Mode>;

  template <class T, core::array_mode AMode> struct structure_config_dto {
    core::structure_dto<T, AMode> structure;
    std::optional<std::array<int, 3>> supercell;
  };

  template <class T> class structure_config {
  public:
    core::structure<T> structure;
    std::optional<std::array<int, 3>> supercell;

    [[nodiscard]] core::structure<T> make_structure() const;

    template <sublattice_mode Mode>
    structure_t<T, Mode> for_mode(const std::vector<sublattice>& composition) const {
      if constexpr (Mode == sublattice_mode::interact) {
        return make_structure();
      } else if constexpr (Mode == sublattice_mode::split) {
        return make_structure().apply_composition_and_decompose(composition);
      } else
        static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split,
                      "unhandled sublattice_mode");
    }

    template <core::array_mode AMode> structure_config_dto<T, AMode> to_dto() const;

    template <core::array_mode AMode>
    static io::result_t<structure_config<T>> from_dto(const structure_config_dto<T, AMode>& input);
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
  result_t<structure_config<T>> validate_structure_input(const structure_input<T>&);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/structure_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_STRUCTURE_H
