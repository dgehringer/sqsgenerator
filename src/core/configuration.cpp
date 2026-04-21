//
// Created by Dominik Gehringer on 21.04.26.
//

#include "sqsgen/core/config.h"

namespace sqsgen::core {

  template <class T> struct configuration<T, SUBLATTICE_MODE_INTERACT, ITERATION_MODE_RANDOM, false>
      : configuration_base<SUBLATTICE_MODE_INTERACT, ITERATION_MODE_RANDOM> {
    seed_t seed;
    structure_config<T> structure;
    sublattice composition;
    std::vector<T> shell_radii;
    shell_weights_t<T> shell_weights;
    cube_t<T> prefactors;
    cube_t<T> pair_weights;
    cube_t<T> target_objective;
    iterations_t iterations;
  };
  template <class T>
  struct configuration<T, SUBLATTICE_MODE_INTERACT, ITERATION_MODE_SYSTEMATIC, false>
      : configuration_base<SUBLATTICE_MODE_INTERACT, ITERATION_MODE_SYSTEMATIC> {
    structure_config<T> structure;
    sublattice composition;
    std::vector<T> shell_radii;
    shell_weights_t<T> shell_weights;
    cube_t<T> prefactors;
    cube_t<T> pair_weights;
    cube_t<T> target_objective;
  };

  template <class T> struct configuration<T, SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM, false>
      : configuration_base<SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM> {
    std::vector<seed_t> seed;
    structure_config<T> structure;
    std::vector<sublattice> composition;
    stl_matrix_t<T> shell_radii;
    std::vector<shell_weights_t<T>> shell_weights;
    std::vector<cube_t<T>> prefactors;
    std::vector<cube_t<T>> pair_weights;
    std::vector<cube_t<T>> target_objective;
    iterations_t iterations;
  };

  template <> struct configuration<float, SUBLATTICE_MODE_INTERACT, ITERATION_MODE_RANDOM, false>;
  template <> struct configuration<double, SUBLATTICE_MODE_INTERACT, ITERATION_MODE_RANDOM, false>;
  template <>
  struct configuration<float, SUBLATTICE_MODE_INTERACT, ITERATION_MODE_SYSTEMATIC, false>;
  template <>
  struct configuration<double, SUBLATTICE_MODE_INTERACT, ITERATION_MODE_SYSTEMATIC, false>;
  template <> struct configuration<float, SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM, false>;
  template <> struct configuration<double, SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM, false>;
}  // namespace sqsgen::core
