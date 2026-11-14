//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include <expected>
#include <type_traits>
#include <variant>
#include <vector>

#include "sqsgen/configuration/composition.h"
#include "sqsgen/configuration/energy_objective.h"
#include "sqsgen/configuration/misc.h"
#include "sqsgen/configuration/pair_targets.h"
#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/configuration/shell_radii.h"
#include "sqsgen/configuration/sro_objective.h"
#include "sqsgen/configuration/structure.h"
#include "sqsgen/configuration/triplet_targets.h"
#include "sqsgen/configuration/triplet_weights.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace io = sqsgen::io;
  namespace core = sqsgen::core;

  template <class T>
    requires std::is_arithmetic_v<T>
  consteval prec arithmethic_to_prec() {
    if constexpr (std::is_same_v<T, float>) {
      return prec::Single;
    } else if constexpr (std::is_same_v<T, double>) {
      return prec::Double;
    } else
      static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>, "unhandled precision");
  }

  template <class T, sublattice_mode Mode> using objective_t
      = std::variant<sro_objective<T, Mode>, energy_objective<T, Mode>>;

  template <class T, sublattice_mode Mode> using objective_input
      = std::optional<std::variant<sro_objective_input<T, Mode>, energy_objective_input<T, Mode>>>;

  template <class T, sublattice_mode Mode> struct configuration_input {
    sqsgen::sublattice_mode sublattice_mode;
    sqsgen::prec prec;
    atol_input<T> atol;
    rtol_input<T> rtol;
    std::optional<bool> pairs;
    std::optional<bool> triplets;
    bin_width_input<T> bin_width;
    peak_isolation_input<T> peak_isolation;
    structure_input<T> structure;
    composition_input composition;
    shell_radii_input<T, Mode> shell_radii;
    iteration_input iteration;           // replaces iteration_mode, iterations, seed
    objective_input<T, Mode> objective;  // replaces pairs, triplets, the four lists
    chunk_size_input chunk_size;
    keep_input keep;
    thread_config_input thread_config;
    max_results_per_objective_input max_results_per_objective;
    seed_input seed;
  };

  template <class T, sublattice_mode Mode> struct configuration {
    sqsgen::sublattice_mode sublattice_mode = Mode;
    sqsgen::prec prec = arithmethic_to_prec<T>();
    atol_t<T> atol;
    rtol_t<T> rtol;
    bool pairs;
    bool triplets;
    bin_width_t<T> bin_width;
    peak_isolation_t<T> peak_isolation;
    structure_config<T> structure;
    std::vector<sublattice> composition;
    shell_radii_t<T, Mode> shell_radii;
    iteration_t iteration;
    objective_t<T, Mode> objective;
    chunk_size_t chunk_size;
    keep_t keep;
    thread_config_t thread_config;
    max_results_per_objective_t max_results_per_objective;
    seed_t seed;

    configuration_input<T, Mode> to_input() const;
  };

  using configuration_input_variant_t
      = std::variant<configuration_input<float, sublattice_mode::interact>,
                     configuration_input<float, sublattice_mode::split>,
                     configuration_input<double, sublattice_mode::interact>,
                     configuration_input<double, sublattice_mode::split>>;

  using configuration_variant_t = std::variant<configuration<float, sublattice_mode::interact>,
                                               configuration<float, sublattice_mode::split>,
                                               configuration<double, sublattice_mode::interact>,
                                               configuration<double, sublattice_mode::split>>;

  std::expected<configuration_input_variant_t, io::parser_error> read_input(std::string_view,
                                                                            io::wire_format);

  result_t<configuration_variant_t> parse_configuration(const configuration_input_variant_t&);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/configuration_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_H
