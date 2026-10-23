//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include <expected>
#include <type_traits>

#include "sqsgen/configuration/arrays.h"
#include "sqsgen/configuration/composition.h"
#include "sqsgen/configuration/misc.h"
#include "sqsgen/configuration/shell_radii.h"
#include "sqsgen/configuration/shell_weights.h"
#include "sqsgen/configuration/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T>
    requires std::is_arithmetic_v<T>
  consteval prec arithmethic_to_prec() {
    if constexpr (std::is_same_v<T, float>) {
      return prec::Single;
    } else if constexpr (std::is_same_v<double, double>) {
      return prec::Double;
    } else
      static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>, "unhandled precision");
  }

  template <class T, sublattice_mode Mode> struct configuration_input {
    sublattice_mode sublattice_mode;
    prec prec;
    atol_input<T> atol;
    rtol_input<T> rtol;
    bin_width_input<T> bin_width;
    peak_isolation_input<T> peak_isolation;
    structure_input<T> structure;
    composition_input composition;
    iteration_mode iteration_mode;
    shell_radii_input<T, Mode> shell_radii;
    shell_weights_input<T, Mode> shell_weights;
    pair_weights_input<T, Mode> pair_weights;
    target_objective_input<T, Mode> target_objective;
    prefactors_input<T, Mode> prefactors;
    iterations_input iterations;
    chunk_size_input chunk_size;
    keep_input keep;
    thread_config_input thread_config;
    max_results_per_objective_input max_results_per_objective;
    seed_input seed;
  };

  template <class T, sublattice_mode Mode> struct configuration {
    sublattice_mode sublattice_mode = Mode;
    prec prec = arithmethic_to_prec<T>();
    atol_t<T> atol;
    rtol_t<T> rtol;
    bin_width_t<T> bin_width;
    peak_isolation_t<T> peak_isolation;
    structure_config<T> structure;
    std::vector<sublattice> composition;
    iteration_mode iteration_mode;
    shell_radii_t<T, Mode> shell_radii;
    shell_weights_t<T, Mode> shell_weights;
    pair_weights_t<T, Mode> pair_weights;
    target_objective_t<T, Mode> target_objective;
    prefactors_t<T, Mode> prefactors;
    iterations_t iterations;
    chunk_size_t chunk_size;
    keep_t keep;
    thread_config_t thread_config;
    max_results_per_objective_t max_results_per_objective;
    seed_t seed;
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

#endif  // SQSGEN_CONFIGURATION_H
