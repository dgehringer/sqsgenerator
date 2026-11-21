//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_OPTIMIZATION_OPTIMIZER_H
#define SQSGEN_OPTIMIZATION_OPTIMIZER_H

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stop_token>
#include <utility>
#include <vector>

#include "sqsgen/configuration.h"
#include "sqsgen/optimization/kernel_data.h"
#include "sqsgen/optimization/make_kernel_data.h"
#include "sqsgen/optimization/objective.h"
#include "sqsgen/optimization/permutation.h"
#include "sqsgen/optimization/result.h"
#include "sqsgen/optimization/result_collection.h"
#include "sqsgen/optimization/runner.h"
#include "sqsgen/optimization/scheduler.h"
#include "sqsgen/optimization/work_source.h"
#include "sqsgen/types.h"

/**
 * @file
 * @brief The optimizer: kernel, hit handling and the driver that ties the modules together.
 *
 * Where each axis is resolved:
 *   iteration mode   template parameter of the kernel; only begin_iteration / advance differ
 *   sublattice mode  make_kernel_data (input) and make_result / finalize_results (output);
 *                    the kernel loops over a runtime list of lattices
 *   objective type   make_kernel_data computes A, B, s; evaluate_lattice picks the evaluator
 *   MPI              optimize(): which work source the threads use, plus the final gather;
 *                    the role comes from run_context (runner.h)
 */

namespace sqsgen::optimization {

  namespace configuration = sqsgen::configuration;

  // ======================================================================= iteration modes

  /**
   * @brief First configuration of a package (and, in random mode, of every block).
   *
   *   random:     configuration = initial, generators = make_stream(seed_r, iteration / B)
   *   systematic: configuration = arrangement with rank = iteration
   */
  template <class T, iteration_mode I>
  void begin_iteration(const lattice_data<T>& lattice, lattice_scratch& scratch,
                       std::uint64_t iteration, std::uint64_t block_size);

  /**
   * @brief Next configuration.
   *
   *   random:      Fisher-Yates shuffle of every range
   *   systematic:  lexicographic successor of the single range
   *   monte carlo: not implemented yet
   */
  template <class T, iteration_mode I>
  void advance(const lattice_data<T>& lattice, lattice_scratch& scratch);

  // ======================================================================= one lattice

  // objective value of each order; nullopt if the order is not used
  template <class T> struct order_objectives {
    std::optional<T> pairs;
    std::optional<T> triplets;
  };

  // counts of the current configuration -> scratch.pair_counts, scratch.triplet_counts
  template <class T> void count_lattice(const lattice_data<T>& lattice, lattice_scratch& scratch);

  // objective of each order from the counts in scratch (count_lattice must run first)
  template <class T> order_objectives<T> evaluate_lattice(const lattice_data<T>& lattice,
                                                          const lattice_scratch& scratch,
                                                          objective_kind kind);

  // alpha = 1 - Nbar f in the canonical cells xi <= eta, mirrored into (eta, xi)
  template <class T> pair_tensor_t<T> pair_sro_from_counts(const pair_tensor_t<count_t>& counts,
                                                           const pair_tensor_t<T>& prefactors);

  // beta = 1 - Nbar f in the canonical cells (the ones to_triplet_sro reads)
  template <class T>
  triplet_tensor_t<T> triplet_sro_from_counts(const triplet_tensor_t<count_t>& counts,
                                              const triplet_tensor_t<T>& prefactors,
                                              const core::triplet_class_mapping_t& classes);

  // ======================================================================= kernel

  // side effects of a hit (objective <= gate): store the result, lower the gate
  template <class S, class T>
  concept hit_sink = requires(S& sink, T objective, const kernel_scratch& scratch) {
    { sink(objective, scratch) };
  };

  // satisfies the kernel concept of work_source.h: run(package, thread) -> report
  template <class T, iteration_mode I, hit_sink<T> Sink> struct optimization_kernel {
    const kernel_data<T>& data;
    std::vector<kernel_scratch>& scratch;  // working memory for each thread
    std::atomic<T>& gate;                  // worst objective that is still kept
    Sink& on_hit;
    std::stop_token stop;  // signal or user stop

    static constexpr std::uint64_t stop_check_interval = 1024;

    // fills report::thread with `thread`
    report run(const package& p, std::size_t thread);
  };

  // ======================================================================= results

  // per-lattice values -> sublattice nesting (interact: the single lattice, split: all)
  template <sublattice_mode Mode, class X>
  sublattice_mode_nesting<X, Mode> from_lattices(std::vector<X>&& per_lattice);

  // result of the configuration currently in `scratch` (counts must be up to date);
  // configurations stay packed and in layout order until finalize_results
  template <class T, sublattice_mode Mode>
  sqs_result<T, Mode> make_result(const kernel_data<T>& data, const kernel_scratch& scratch);

  // packed configuration in layout order -> atomic numbers in input site order
  configuration_t to_input_order(const configuration_t& packed, const std::vector<site_t>& order,
                                 const std::vector<specie_t>& species);

  template <class T, sublattice_mode Mode> using result_snapshot
      = std::vector<std::pair<T, std::vector<sqs_result<T, Mode>>>>;

  // applies to_input_order to every stored configuration
  template <class T, sublattice_mode Mode>
  result_snapshot<T, Mode> finalize_results(const kernel_data<T>& data,
                                            result_snapshot<T, Mode>&& snapshot);

  // sum of iterations and hits; seconds = the longest-running thread
  statistics merge_statistics(const std::vector<kernel_scratch>& scratch);

  // iterations: config.iterations (random) or the number of arrangements (systematic)
  template <class T, iteration_mode I>
  std::uint64_t total_iterations(const kernel_data<T>& data,
                                 configuration::iterations_t iterations);

  template <class T, sublattice_mode Mode> struct optimization_output {
    result_snapshot<T, Mode> results;  // best objectives first, empty on MPI worker ranks
    statistics stats;
  };

  /**
   * @brief Run the optimization.
   *
   *   single: threads use tracked_source(local_source); the main thread publishes progress
   *   head:   same for its threads; the main thread serves remote requests (serve_head) and
   *           publishes progress between requests
   *   worker: threads use mpi_source; the main thread forwards requests (serve_worker)
   * After the threads are joined, MPI runs gather the results on the head.
   */
  template <class T, sublattice_mode Mode, iteration_mode I>
  optimization_output<T, Mode> optimize(const configuration::configuration<T, Mode>& config,
                                        const run_context& context);

  // runtime iteration mode -> template; the only place the combinations are instantiated
  template <class T, sublattice_mode Mode>
  optimization_output<T, Mode> optimize(const configuration::configuration<T, Mode>& config,
                                        const run_context& context);

}  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/optimization_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_OPTIMIZER_H
