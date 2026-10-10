//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include <optional>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen {

  template <class T, SublatticeMode S, IterationMode I> struct configuration_base {
    static constexpr SublatticeMode sublattice_mode = S;
    static constexpr IterationMode iteration_mode = I;
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

}  // namespace sqsgen

#endif  // SQSGEN_CONFIGURATION_H
