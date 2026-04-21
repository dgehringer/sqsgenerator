//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include <cstdint>
#include <optional>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

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

  template <SublatticeMode smode, IterationMode imode> struct configuration_base {
    SublatticeMode sublattice_mode{smode};
    IterationMode iteration_mode{imode};
    std::size_t keep;
    std::optional<std::size_t> max_results_per_objective;
    thread_config_t thread_config;
    iterations_t chunk_size{};
  };

  template <class T, SublatticeMode smode, IterationMode imode, bool mpi_mode = false>
  struct configuration;

}  // namespace sqsgen::core

#endif  // SQSGEN_CONFIGURATION_H
