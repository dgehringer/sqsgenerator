//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include <optional>

#include "io/json.h"
#include "sqsgen/io/parser/structure_config.h"

namespace sqsgen {

  template <class T> class structure_config {
  public:
    static constexpr auto DEFAULT_SUPERCELL = std::array{1,1,1};
    lattice_t<T> lattice;
    coords_t<T> coords;
    configuration_t species;
    std::optional<std::array<int, 3>> supercell = std::nullopt;

    core::structure<T> structure(bool supercell = true) {
      core::structure<T> structure(lattice, coords, species);
      if (supercell) {
        auto [a, b, c] = this->supercell.value_or(DEFAULT_SUPERCELL);
        structure = structure.supercell(a, b, c);
      }
      return structure;
    }
  };

  class composition_config {
    public:

  };



  template <class T> struct configuration {
    structure_config<T> structure;
  };

}  // namespace sqsgen

#endif  // SQSGEN_CONFIGURATION_H
