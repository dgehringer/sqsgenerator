//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include <optional>

#include "io/json.h"
#include "sqsgen/io/parser/structure.h"

namespace sqsgen {

  template <class T> class structure_configuration {
  public:
    lattice_t<T> lattice;
    coords_t<T> coords;
    configuration_t species;
    std::optional<std::array<int, 3>> supercell = std::nullopt;
    std::optional<std::vector<int>> which = std::nullopt;

    core::structure<T> structure(bool supercell = true, bool which = true) {
      core::structure<T> structure(lattice, coords, species);
      if (supercell) {
        auto [a, b, c] = this->supercell.value_or(std::array{1, 1, 1});
        structure = structure.supercell(a, b, c);
      }
      if (which) {
        auto slice = this->which.value_or(std::vector<int>{});
        if (!slice.empty()) structure = structure.sliced(slice);
      }
      return structure;
    }
  };

  template <class T> struct configuration {};

}  // namespace sqsgen

#endif  // SQSGEN_CONFIGURATION_H
