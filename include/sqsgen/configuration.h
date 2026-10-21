//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include <optional>
#include <type_traits>

#include "sqsgen/core/structure.h"
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

  template <class T, sublattice_mode SMode> struct configuration_base {
    static constexpr sublattice_mode sublattice_mode;
    static constexpr iteration_mode iteration_mode = IMode;
    static constexpr prec prec = arithmethic_to_prec<T>();
    ShellRadiiDetection radii_detection_mode = SHELL_RADII_DETECTION_PEAK;
    structure_config<T> structure;
    std::size_t keep;
    thread_config_t thread_config;
    std::optional<std::size_t> max_results_per_objective;
  };

  template <class T, SublatticeMode S, IterationMode I> struct configuration;

  template <class T> struct configuration<T, SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM>
      : configuration_base<T, SUBLATTICE_MODE_SPLIT, ITERATION_MODE_RANDOM> {
    std::vector<sublattice> composition;
    nested_t<T, 2> shell_radii;
    std::vector<shell_weights_t<T>> shell_weights;
    std::vector<cube_t<T>> prefactors;
    std::vector<cube_t<T>> pair_weights;
    std::vector<cube_t<T>> target_objective;
    iterations_t iterations;
  };

}  // namespace sqsgen::configuration

#endif  // SQSGEN_CONFIGURATION_H
