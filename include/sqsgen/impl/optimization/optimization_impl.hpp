//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_OPTIMIZER_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_OPTIMIZER_IMPL_HPP

#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <variant>

#include "sqsgen/optimization/optimization.h"

#ifdef SQSGEN_MPI
#  include "sqsgen/optimization/mpi/mpi_source.h"
#endif

namespace sqsgen::optimization {

  namespace configuration = sqsgen::configuration;

  template <class T, iteration_mode I>
  void begin_iteration(const lattice_data<T>& lattice, lattice_scratch& scratch,
                       std::uint64_t iteration, std::uint64_t block_size) {
    scratch.configuration = lattice.species_packed;
    if constexpr (I == iteration_mode::random) {
      seed_streams(scratch.generators, lattice.seeds, iteration / block_size);
    } else {
      unrank(view(scratch.configuration, lattice.ranges.front()), rank_t{iteration});
    }
  }

  template <class T, iteration_mode I>
  void advance(const lattice_data<T>& lattice, lattice_scratch& scratch) {
    if constexpr (I == iteration_mode::random) {
      shuffle(scratch.configuration, lattice.ranges, scratch.generators);
    } else {
      next(view(scratch.configuration, lattice.ranges.front()));
    }
  }

  template <class T> void count_lattice(const lattice_data<T>& lattice, lattice_scratch& scratch) {
    if (lattice.pairs)
      count_pairs(scratch.pair_counts, lattice.pairs->pairs, scratch.configuration);
    if (lattice.triplets)
      count_triplets(scratch.triplet_counts, lattice.triplets->triplets, scratch.configuration);
  }

  template <class T> order_objectives<T> evaluate_lattice(const lattice_data<T>& lattice,
                                                          const lattice_scratch& scratch,
                                                          objective_kind kind) {
    order_objectives<T> out;
    if (lattice.pairs) {
      const auto& p = *lattice.pairs;
      out.pairs = kind == objective_kind::sro
                      ? compute_pair_sro_objective<T>(scratch.pair_counts, p.terms)
                      : compute_pair_energy_objective<T>(scratch.pair_counts, p.terms);
    }
    if (lattice.triplets) {
      const auto& t = *lattice.triplets;
      out.triplets
          = kind == objective_kind::sro
                ? compute_triplet_sro_objective<T>(scratch.triplet_counts, t.terms, t.classes)
                : compute_triplet_energy_objective<T>(scratch.triplet_counts, t.terms, t.classes);
    }
    return out;
  }

  template <class T> pair_tensor_t<T> pair_sro_from_counts(const pair_tensor_t<count_t>& counts,
                                                           const pair_tensor_t<T>& prefactors) {
    pair_tensor_t<T> alpha(counts.dimensions());
    detail::for_each_pair_term(
        counts, [&](Eigen::Index s, Eigen::Index xi, Eigen::Index eta, count_t nbar) {
          const T value = T{1} - static_cast<T>(nbar) * prefactors(s, xi, eta);
          alpha(s, xi, eta) = value;
          alpha(s, eta, xi) = value;
        });
    return alpha;
  }

  template <class T>
  triplet_tensor_t<T> triplet_sro_from_counts(const triplet_tensor_t<count_t>& counts,
                                              const triplet_tensor_t<T>& prefactors,
                                              const core::triplet_class_mapping_t& classes) {
    triplet_tensor_t<T> beta(counts.dimensions());
    beta.setConstant(std::numeric_limits<T>::quiet_NaN());  // non-canonical cells: unused
    detail::for_each_triplet_term(
        counts, classes,
        [&](Eigen::Index c, Eigen::Index xi, Eigen::Index eta, Eigen::Index zeta, count_t nbar) {
          beta(c, xi, eta, zeta) = T{1} - static_cast<T>(nbar) * prefactors(c, xi, eta, zeta);
        });
    return beta;
  }

  template <class T, iteration_mode I, hit_sink<T> Sink>
  report optimization_kernel<T, I, Sink>::run(const package& p, std::size_t thread) {
    kernel_scratch& t = scratch[thread];
    const auto start_time = std::chrono::steady_clock::now();
    if constexpr (I == iteration_mode::random) assert(p.start % data.block_size == 0);

    std::uint64_t done = 0;
    for (std::uint64_t i = p.start; i < p.start + p.size; ++i) {
      // random: every block starts from the initial configuration with fresh streams
      const bool fresh
          = (i == p.start) || (I == iteration_mode::random && i % data.block_size == 0);

      T value{0};
      for (std::size_t k = 0; k < data.lattices.size(); ++k) {
        const lattice_data<T>& lattice = data.lattices[k];
        lattice_scratch& s = t.lattices[k];
        if (fresh) begin_iteration<T, I>(lattice, s, i, data.block_size);
        // systematic: the first configuration of a package is the unranked one itself
        if (I == iteration_mode::random || !fresh) advance<T, I>(lattice, s);
        count_lattice(lattice, s);
        const order_objectives<T> o = evaluate_lattice(lattice, s, data.objective);
        value += o.pairs.value_or(T{0}) + o.triplets.value_or(T{0});
      }
      ++done;

      if (value <= gate.load(std::memory_order_relaxed)) {
        ++t.stats.hits;
        on_hit(value, t);
      }
      if (done % stop_check_interval == 0 && stop.stop_requested()) break;
    }

    const double seconds
        = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
    t.stats.iterations += done;
    t.stats.seconds += seconds;
    return report{
        .best = static_cast<double>(gate.load(std::memory_order_relaxed)),
        .rate
        = seconds > 0.0 ? std::optional<double>{static_cast<double>(done) / seconds} : std::nullopt,
        .done = done,
        .stop = stop.stop_requested(),
    };
  }

  // ======================================================================= results

  template <sublattice_mode Mode, class X>
  sublattice_mode_nesting<X, Mode> from_lattices(std::vector<X>&& per_lattice) {
    if constexpr (Mode == sublattice_mode::interact) {
      assert(per_lattice.size() == 1);
      return std::move(per_lattice.front());
    } else {
      return std::move(per_lattice);
    }
  }

  template <class T, sublattice_mode Mode>
  sqs_result<T, Mode> make_result(const kernel_data<T>& data, const kernel_scratch& scratch) {
    std::vector<configuration_t> configurations;
    std::vector<T> pair_objectives, triplet_objectives;
    std::vector<pair_sro_t<T>> pair_sro;
    std::vector<triplet_sro_t<T>> triplet_sro;

    for (std::size_t k = 0; k < data.lattices.size(); ++k) {
      const lattice_data<T>& lattice = data.lattices[k];
      const lattice_scratch& s = scratch.lattices[k];
      configurations.push_back(s.configuration);

      const order_objectives<T> o = evaluate_lattice(lattice, s, data.objective);
      if (lattice.pairs) {
        const auto& p = *lattice.pairs;
        pair_objectives.push_back(o.pairs.value());
        pair_sro.push_back(to_pair_sro(pair_sro_from_counts(s.pair_counts, p.prefactors), p.shells,
                                       lattice.species));
      }
      if (lattice.triplets) {
        const auto& t = *lattice.triplets;
        triplet_objectives.push_back(o.triplets.value());
        triplet_sro.push_back(
            to_triplet_sro(triplet_sro_from_counts(s.triplet_counts, t.prefactors, t.classes),
                           t.classes, lattice.species));
      }
    }

    sqs_result<T, Mode> result;
    result.configuration = from_lattices<Mode>(std::move(configurations));
    if (!pair_objectives.empty())
      result.pairs = order_result<T, Mode, pair_sro_t<T>>{
          .objective = from_lattices<Mode>(std::move(pair_objectives)),
          .sro = from_lattices<Mode>(std::move(pair_sro)),
      };
    if (!triplet_objectives.empty())
      result.triplets = order_result<T, Mode, triplet_sro_t<T>>{
          .objective = from_lattices<Mode>(std::move(triplet_objectives)),
          .sro = from_lattices<Mode>(std::move(triplet_sro)),
      };
    return result;
  }

  inline configuration_t to_input_order(const configuration_t& packed,
                                        const std::vector<site_t>& order,
                                        const std::vector<specie_t>& species) {
    configuration_t out(packed.size());
    for (std::size_t k = 0; k < packed.size(); ++k) out[order[k]] = species[packed[k]];
    return out;
  }

  template <class T, sublattice_mode Mode>
  result_snapshot<T, Mode> finalize_results(const kernel_data<T>& data,
                                            result_snapshot<T, Mode>&& snapshot) {
    for (auto& [objective, results] : snapshot)
      for (auto& r : results) {
        if constexpr (Mode == sublattice_mode::interact) {
          const auto& lattice = data.lattices.front();
          r.configuration = to_input_order(r.configuration, lattice.order, lattice.species);
        } else {
          for (std::size_t k = 0; k < data.lattices.size(); ++k) {
            const auto& lattice = data.lattices[k];
            r.configuration[k] = to_input_order(r.configuration[k], lattice.order, lattice.species);
          }
        }
      }
    return std::move(snapshot);
  }

  inline statistics merge_statistics(const std::vector<kernel_scratch>& scratch) {
    statistics out;
    for (const kernel_scratch& t : scratch) {
      out.iterations += t.stats.iterations;
      out.hits += t.stats.hits;
      out.seconds = std::max(out.seconds, t.stats.seconds);
    }
    return out;
  }

  template <class T, iteration_mode I>
  std::uint64_t total_iterations(const kernel_data<T>& data,
                                 configuration::iterations_t iterations) {
    if constexpr (I == iteration_mode::random) {
      // nullopt: the user left the iteration count unset, fall back to the configured default
      return iterations.value_or(configuration::defaults::iterations_default);
    } else {
      // systematic: one lattice, one range (checked during validation); the number of
      // arrangements fits into 64 bits (checked during validation as well)
      const lattice_data<T>& lattice = data.lattices.front();
      return static_cast<std::uint64_t>(
          num_permutations(view(lattice.species_packed, lattice.ranges.front())));
    }
  }

  // Iteration count requested by the active mode: the alternative's `iterations`
  // (nullopt falls back to the default), or nullopt for systematic (which derives
  // its count from the number of arrangements).
  inline configuration::iterations_t requested_iterations(const configuration::iteration_t& it) {
    return std::visit(
        []<class M>(const M& mode) -> configuration::iterations_t {
          if constexpr (std::is_same_v<M, configuration::systematic>)
            return std::nullopt;
          else
            return mode.iterations;
        },
        it);
  }

  template <class T, sublattice_mode Mode, iteration_mode I>
  optimization_output<T, Mode> optimize(const configuration::configuration<T, Mode>& config,
                                        const run_context& context) {
    const kernel_data<T> data = make_kernel_data(config);

    std::vector<kernel_scratch> scratch;
    scratch.reserve(context.num_threads);
    for (std::size_t t = 0; t < context.num_threads; ++t) scratch.push_back(make_scratch(data));

    std::atomic<T> gate{std::numeric_limits<T>::infinity()};
    sqs_result_collection<T, Mode> results;

    // a hit: keep at most max_results_per_objective per objective, then lower the gate to the
    // worst objective among the `keep` best (infinity while fewer than `keep` are stored)
    auto on_hit = [&](T objective, const kernel_scratch& t) {
      if (results.count_for_objective(objective) >= config.max_results_per_objective) return;
      results.insert(make_result<T, Mode>(data, t));
      atomic_min(gate, results.nth_best_objective(config.keep));
    };

    optimization_kernel<T, I, decltype(on_hit)> kernel{data, scratch, gate, on_hit, context.stop};

    const scheduler_settings settings{
        .total = total_iterations<T, I>(data, requested_iterations(config.iteration)),
        .block_size = data.block_size,
        .num_consumers = context.num_threads + context.num_remote_consumers,
        .target_seconds = 1.0,
    };

    switch (context.role) {
      case mpi_role::single: {
        shared_scheduler shared(settings);
        local_source inner{shared};
        progress_tracker tracker;
        reset(tracker, settings.total);
        // tracked_source folds each thread's report into `tracker` once per package
        tracked_source<local_source> source{inner, tracker, /*rank=*/0};

        // publish progress from a side thread while the compute threads run; it wakes
        // every progress_interval and once more right after the run for a final frame
        std::stop_source publish_stop;
        std::jthread publisher;
        if (context.on_progress)
          publisher = std::jthread([&] {
            std::mutex m;
            std::condition_variable_any cv;
            std::unique_lock lock(m);
            while (!cv.wait_for(lock, publish_stop.get_token(), context.progress_interval,
                                [&] { return publish_stop.stop_requested(); }))
              context.on_progress(snapshot(tracker));
          });

        execute(source, kernel, context.num_threads);

        if (publisher.joinable()) {
          publish_stop.request_stop();
          publisher.join();
        }
        if (context.on_progress) context.on_progress(snapshot(tracker));
        break;
      }
#ifdef SQSGEN_MPI
      case mpi_role::head: {
        shared_scheduler shared(settings);
        local_source source{shared};
        std::jthread workers([&] { execute(source, kernel, context.num_threads); });
        mpi::serve_head(shared, gate, context.num_remote_consumers, MPI_COMM_WORLD);
        break;  // workers joins here
      }
      case mpi_role::worker: {
        mpi::mpi_source source;
        std::jthread workers([&] { execute(source, kernel, context.num_threads); });
        mpi::serve_worker(source, gate, context.num_threads, /*head=*/0, MPI_COMM_WORLD);
        break;  // workers joins here
      }
#else
      default:
        assert(false && "built without MPI");
        break;
#endif
    }

    statistics stats = merge_statistics(scratch);
#ifdef SQSGEN_MPI
    if (context.role != role::single) mpi::gather_results(results, stats, MPI_COMM_WORLD);
#endif

    return optimization_output<T, Mode>{
        .results = finalize_results<T, Mode>(data, results.snapshot()),
        .stats = stats,
    };
  }

  template <class T, sublattice_mode Mode>
  optimization_output<T, Mode> optimize(const configuration::configuration<T, Mode>& config,
                                        const run_context& context) {
    return std::visit(
        [&]<class M>(const M&) -> optimization_output<T, Mode> {
          if constexpr (std::is_same_v<M, configuration::systematic>)
            return optimize<T, Mode, iteration_mode::systematic>(config, context);
          else
            // random and monte_carlo both iterate by random shuffling; the Monte-Carlo
            // acceptance kernel is not implemented yet, so both use the random kernel.
            return optimize<T, Mode, iteration_mode::random>(config, context);
        },
        config.iteration);
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_OPTIMIZER_IMPL_HPP
