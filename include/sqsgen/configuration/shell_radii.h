//
// Created by Dominik Gehringer on 02.12.24.
//

#ifndef SQSGEN_CONFIGURATION_SHELL_RADII_H
#define SQSGEN_CONFIGURATION_SHELL_RADII_H

#include <variant>
#include <vector>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T> static constexpr T atol_default = 1.0e-3;
  template <class T> static constexpr T rtol_default = 1.0e-5;
  template <class T> static constexpr T bin_width_default = 0.05;       // Angstrom
  template <class T> static constexpr T peak_isolation_default = 0.25;  // Angstrom

  template <class T, sublattice_mode Mode> using structure_input
      = sublattice_mode_nesting<core::structure<T>, Mode>;

  template <class T, sublattice_mode Mode> using shell_radii_spec
      = sublattice_mode_nesting<std::vector<T>, Mode>;

  template <class T, sublattice_mode Mode> using shell_radii = shell_radii_spec<T, Mode>;

  template <class T, sublattice_mode Mode> using shell_radii_input
      = std::variant<shell_radii_detection, shell_radii_spec<T, Mode>>;

  template <class T, sublattice_mode Mode>
  std::expected<shell_radii<T, Mode>, configuration_error> validate_shell_radii_input(
      const shell_radii_input<T, Mode>&, const structure_input<T, Mode>& structure, T atol, T rtol,
      T bin_width, T peak_isolation);

}  // namespace sqsgen::configuration

#endif  // SQSGEN_CONFIGURATION_SHELL_RADII_H
