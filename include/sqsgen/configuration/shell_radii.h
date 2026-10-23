//
// Created by Dominik Gehringer on 02.12.24.
//

#ifndef SQSGEN_CONFIGURATION_SHELL_RADII_H
#define SQSGEN_CONFIGURATION_SHELL_RADII_H

#include <variant>
#include <vector>

#include "sqsgen/configuration/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T, sublattice_mode Mode> using shell_radii_spec
      = sublattice_mode_nesting<std::vector<T>, Mode>;

  template <class T, sublattice_mode Mode> using shell_radii_t = shell_radii_spec<T, Mode>;

  template <class T, sublattice_mode Mode> using shell_radii_input
      = std::optional<std::variant<shell_radii_detection, shell_radii_spec<T, Mode>>>;

  template <class T, sublattice_mode Mode>
  std::expected<shell_radii_t<T, Mode>, configuration_error> validate_shell_radii_input(
      const shell_radii_input<T, Mode>&, const structure_t<T, Mode>& structure, T atol, T rtol,
      T bin_width, T peak_isolation);

}  // namespace sqsgen::configuration

#endif  // SQSGEN_CONFIGURATION_SHELL_RADII_H
