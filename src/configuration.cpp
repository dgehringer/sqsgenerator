

#include "sqsgen/configuration.h"

#include <optional>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen {

  template <class T> core::structure<T> structure_config<T>::structure(bool supercell) const {
    core::structure<T> structure(lattice, coords, species);
    if (supercell) {
      auto [a, b, c] = this->supercell;
      structure = structure.supercell(a, b, c);
    }
    return structure;
  }

  template <class T> struct configuration<T, SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM>
      : configuration_base<T, SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM> {
    seed_t seed;
    std::vector<sublattice> composition;
    std::optional<nested_t<T, 2>::type> shell_radii;
    std::optional<std::vector<shell_weights_t<T>>> shell_weights;
    std::optional<std::vector<cube_t<T>>> prefactors;
    std::optional<std::vector<cube_t<T>>> pair_weights;
    std::optional<std::vector<cube_t<T>>> target_objective;
    std::optional<iterations_t> iterations;
  };

  template <class T> struct configuration<T, SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM>
      : configuration_base<T, SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM> {
    seed_t seed;
    std::vector<sublattice> composition;
    stl_matrix_t<T> shell_radii;
    std::vector<shell_weights_t<T>> shell_weights;
    std::vector<cube_t<T>> prefactors;
    std::vector<cube_t<T>> pair_weights;
    std::vector<cube_t<T>> target_objective;
    std::optional<iterations_t> iterations;
  };

};  // namespace sqsgen
