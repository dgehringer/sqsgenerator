//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include "sqsgen/core/structure.h"


namespace sqsgen {

  using composition_t = std::map<specie_t, usize_t>;

  struct sublattice {
    vset<usize_t> sites;
    composition_t composition;
  };

  template <class T> class structure_config {
  public:
    lattice_t<T> lattice;
    coords_t<T> coords;
    configuration_t species;
    std::array<int, 3> supercell;


    core::structure<T> structure(bool supercell = true) const {
      core::structure<T> structure(lattice, coords, species);
      if (supercell) {
        auto [a, b, c] = this->supercell;
        structure = structure.supercell(a, b, c);
      }
      return structure;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(structure_config, lattice, coords, species, supercell);
  };

  template <class T> struct configuration {
    structure_config<T> structure;
    std::vector<sublattice> composition;
    std::vector<T> shell_radii;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(configuration, structure, composition);
  };


}  // namespace sqsgen

#endif  // SQSGEN_CONFIGURATION_H
