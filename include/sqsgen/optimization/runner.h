//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_OPTIMIZATION_RUNNER_H
#define SQSGEN_OPTIMIZATION_RUNNER_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <stop_token>
#include <vector>

#include "sqsgen/configuration/misc.h"
#include "sqsgen/optimization/scheduler.h"
#include "sqsgen/optimization/work_source.h"
#include "sqsgen/types.h"

/**
 * @file
 * @brief How a run executes, independent of what it computes.
 *
 * The configuration says what to compute; the run context says how: MPI role, number of
 * threads, how a stop arrives and where progress goes. Frontends (CLI, Python, WASM) build it
 * once and pass it to optimize(). No MPI code here; the MPI factory is in
 * sqsgen/optimization/mpi/runner.h.
 */

namespace sqsgen::optimization {

  using sqsgen::configuration::thread_config_t;

  // ======================================================================= progress

  // one compute thread, as seen by the process that runs the scheduler
  struct thread_progress {
    std::uint32_t thread;  // index of the thread within its rank
    double rate;           // iterations per second, from its last report
    std::uint64_t done;    // iterations completed by this thread
  };

  // one rank (the single process without MPI)
  struct rank_progress {
    std::uint32_t rank;  // 0 without MPI
    double rate;         // sum of the rates of its threads
    std::vector<thread_progress> threads;
  };

  // snapshot of the whole run; values change when a thread reports, i.e. once per package
  struct progress {
    std::uint64_t done;                       // iterations completed, all ranks
    std::uint64_t total;                      // iterations of the run
    double elapsed_seconds;                   // since the run started
    double rate;                              // iterations per second, all ranks
    std::optional<double> remaining_seconds;  // (total - done) / rate; nullopt before any rate
    std::vector<rank_progress> ranks;         // per rank and per thread
    std::optional<double> best;               // best objective so far; nullopt before a hit
    std::uint64_t since_best;                 // iterations completed since best last improved
  };

  // called periodically on the process that runs the scheduler (single process or MPI head),
  // from its main thread, never from a compute thread; must return quickly
  using progress_callback = std::function<void(const progress&)>;

  // collects the reports of all threads (local and, on the MPI head, remote)
  struct progress_tracker {
    mutable std::mutex mutex;  // mutable: snapshot() locks it on a const tracker
    std::chrono::steady_clock::time_point start;
    std::uint64_t total{0};
    std::vector<rank_progress> ranks;  // indexed by rank
    std::optional<double> best;
    std::uint64_t done_at_best{0};  // value of done when best last improved
  };

  // start a fresh run: reset the clock and total, clear the per-rank and best state
  void reset(progress_tracker& tracker, std::uint64_t total);

  // fold one report into the tracker, under `rank` and its within-rank `thread` index
  void record(progress_tracker& tracker, std::uint32_t rank, std::uint32_t thread, const report& r);

  // a consistent copy of the current progress; safe to call while threads report
  progress snapshot(const progress_tracker& tracker);

  /**
   * @brief Work source that records every report before passing it on.
   *
   * Wraps the source of the local threads (local_source on a single process and on the head).
   * Remote reports are recorded in serve_head. Once per package, outside the hot loop.
   */
  template <work_source W> struct tracked_source {
    W& inner;
    progress_tracker& tracker;
    std::uint32_t rank;  // 0 without MPI, 0 on the head

    std::optional<package> next(const report& r, std::size_t thread);
  };

  // ======================================================================= run context

  enum class mpi_role : std::uint8_t { single, head, worker };  // single = no MPI

  struct run_context {
    mpi_role role{mpi_role::single};
    std::size_t num_threads{1};           // threads of this process
    std::size_t num_remote_consumers{0};  // head only: threads of all other ranks
    std::stop_token stop;                 // stop request from the frontend
    progress_callback on_progress{};      // empty: no progress reporting
    std::chrono::milliseconds progress_interval{1000};
  };

  /**
   * @brief Threads this process runs.
   *
   * The explicit thread_config value when set (threads per rank); otherwise (auto) the hardware
   * thread count shared among the @p ranks_on_node co-located ranks, so they do not oversubscribe.
   * Never fewer than one.
   */
  std::size_t resolve_num_threads(const thread_config_t& threads, std::size_t ranks_on_node);

  /**
   * @brief Run context without MPI (Python, WASM, single-process CLI).
   *
   * @param threads      Validated thread configuration; auto uses all hardware threads.
   * @param stop         Token of the frontend's stop source.
   * @param on_progress  Optional progress callback.
   */
  run_context make_local_run_context(const thread_config_t& threads, std::stop_token stop,
                                     progress_callback on_progress = {});

}  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/runner_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_RUNNER_H
