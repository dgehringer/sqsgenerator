//
// Created by Dominik Gehringer on 02.01.25.
//

#ifndef SQSGEN_CONFIGURATION_ARRAYS_H
#define SQSGEN_CONFIGURATION_ARRAYS_H

#include <optional>
#include <variant>

#include "sqsgen/configuration/shell_radii.h"
#include "sqsgen/configuration/shell_weights.h"
#include "sqsgen/configuration/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {
  template <class T, sublattice_mode Mode> using pair_weights_t
      = sublattice_mode_nesting<cube_t<T>, Mode>;
  template <class T, sublattice_mode Mode> using target_objective_t
      = sublattice_mode_nesting<cube_t<T>, Mode>;
  template <class T, sublattice_mode Mode> using prefactors_t
      = sublattice_mode_nesting<cube_t<T>, Mode>;

  template <class T> using array_spec = std::variant<T, core::nested_t<T, 2>, core::nested_t<T, 3>>;

  template <class T, sublattice_mode Mode> using pair_weights_input
      = std::optional<sublattice_mode_nesting<array_spec<T>, Mode>>;

  template <class T, sublattice_mode Mode> using target_objective_input
      = std::optional<sublattice_mode_nesting<array_spec<T>, Mode>>;

  template <class T, sublattice_mode Mode> using prefactors_input
      = std::optional<sublattice_mode_nesting<array_spec<T>, Mode>>;

  template <class T, sublattice_mode Mode>
  result_t<pair_weights_t<T, Mode>> validate_pair_weights_input(const pair_weights_input<T, Mode>&,
                                                                const structure_t<T, Mode>&,
                                                                const shell_weights_t<T, Mode>&,
                                                                const shell_radii_t<T, Mode>&);

  template <class T, sublattice_mode Mode>
  result_t<target_objective_t<T, Mode>> validate_target_objective_input(
      const target_objective_input<T, Mode>&, const structure_t<T, Mode>&,
      const shell_weights_t<T, Mode>&, const shell_radii_t<T, Mode>&);

  template <class T, sublattice_mode Mode>
  result_t<prefactors_t<T, Mode>> validate_prefactors_input(const prefactors_input<T, Mode>&,
                                                            const structure_t<T, Mode>&,
                                                            const shell_weights_t<T, Mode>&,
                                                            const shell_radii_t<T, Mode>&);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/arrays_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_ARRAYS_H
