//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include "sqsgen/core/structure.h"

namespace sqsgen::core {

  template <class T> class structure_config {
  public:
    lattice_t<T> lattice;
    coords_t<T> coords;
    configuration_t species;
    std::array<int, 3> supercell{1, 1, 1};

    core::structure<T> structure(bool supercell = true) const {
      core::structure<T> structure(lattice, coords, species);
      if (supercell) {
        auto [a, b, c] = this->supercell;
        structure = structure.supercell(a, b, c);
      }
      return structure;
    }
  };

  template <class T> struct configuration {
    SublatticeMode sublattice_mode;
    IterationMode iteration_mode;
    structure_config<T> structure;
    std::vector<sublattice> composition;
    stl_matrix_t<T> shell_radii;
    std::vector<shell_weights_t<T>> shell_weights;
    std::vector<cube_t<T>> prefactors;
    std::vector<cube_t<T>> pair_weights;
    std::vector<cube_t<T>> target_objective;
    std::optional<iterations_t> iterations;
    iterations_t chunk_size{};
    thread_config_t thread_config;
    std::size_t keep;
    std::optional<std::size_t> max_results_per_objective;
  };

}  // namespace sqsgen::core

#endif  // SQSGEN_CONFIGURATION_H
