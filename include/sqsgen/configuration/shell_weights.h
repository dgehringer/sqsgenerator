//
// Created by Dominik Gehringer on 02.12.24.
//

#ifndef SQSGEN_CONFIGURATION_SHELL_WEIGHTS_H
#define SQSGEN_CONFIGURATION_SHELL_WEIGHTS_H

#include <cstddef>
#include <map>
#include <string>

#include "sqsgen/configuration/shell_radii.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T> using shell_weight_spec = std::map<std::string, T>;

  template <class T, sublattice_mode Mode> using shell_weights_input
      = std::optional<sublattice_mode_nesting<shell_weight_spec<T>, Mode>>;

  template <class T, sublattice_mode Mode> using shell_weights_t
      = sublattice_mode_nesting<std::map<std::size_t, T>, Mode>;

  template <class T, sublattice_mode Mode>
  result_t<shell_weights_t<T, Mode>> validate_shell_weight_input(
      const shell_weights_input<T, Mode>&, const shell_radii_t<T, Mode>& shell_radii);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/shell_weights_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_SHELL_WEIGHTS_H
