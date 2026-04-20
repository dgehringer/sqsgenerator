//
// Created by Dominik Gehringer on 20.04.26.
//

#ifndef SQSGENERATOR_CLI_STRUCTURE_UTILS_H
#define SQSGENERATOR_CLI_STRUCTURE_UTILS_H

#include "sqsgen/types.h"

namespace sqsgen::core::structure_utils {

  template <class T>
  matrix_t<T> distance_matrix(const lattice_t<T> &lattice, const coords_t<T> &frac_coords);

  template <class T> shell_matrix_t shell_matrix(matrix_t<T> const &distance_matrix,
                                                 std::vector<T> const &dists, T atol, T rtol);

  template <class T> cube_t<T> compute_prefactors(shell_matrix_t const &shell_matrix,
                                                  shell_weights_t<T> const &weights,
                                                  configuration_t const &configuration);

  std::size_t num_species(configuration_t const &configuration);

  counter<specie_t> count_species(configuration_t const &configuration);

}  // namespace sqsgen::core::structure_utils

#endif  // SQSGENERATOR_CLI_STRUCTURE_UTILS_H
