//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_RUNNER_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_RUNNER_IMPL_HPP

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

#include "sqsgen/optimization/runner.h"

namespace sqsgen::optimization {

  inline void reset(progress_tracker& tracker, std::uint64_t total) {
    std::scoped_lock lock(tracker.mutex);
    tracker.start = std::chrono::steady_clock::now();
    tracker.total = total;
    tracker.ranks.clear();
    tracker.best.reset();
    tracker.done_at_best = 0;
  }

  inline void record(progress_tracker& tracker, std::uint32_t rank, std::uint32_t thread,
                     const report& r) {
    std::scoped_lock lock(tracker.mutex);

    if (static_cast<std::size_t>(rank) >= tracker.ranks.size())
      tracker.ranks.resize(static_cast<std::size_t>(rank) + 1);
    rank_progress& rp = tracker.ranks[rank];
    rp.rank = rank;

    // grow the thread table on first sight of a thread; number the new slots
    if (static_cast<std::size_t>(thread) >= rp.threads.size()) {
      const std::size_t first = rp.threads.size();
      rp.threads.resize(static_cast<std::size_t>(thread) + 1);
      for (std::size_t t = first; t < rp.threads.size(); ++t)
        rp.threads[t].thread = static_cast<std::uint32_t>(t);
    }
    thread_progress& tp = rp.threads[thread];
    tp.rate = r.rate.value_or(0.0);  // nullopt: no rate measured in the last package
    tp.done += r.done;

    // a rank's rate is the sum of the latest rates of its threads
    double rank_rate = 0.0;
    for (const thread_progress& t : rp.threads) rank_rate += t.rate;
    rp.rate = rank_rate;

    // best objective across all ranks, and the work done when it last improved
    if (std::isfinite(r.best) && (!tracker.best || r.best < tracker.best.value())) {
      tracker.best = r.best;
      std::uint64_t done = 0;
      for (const rank_progress& x : tracker.ranks)
        for (const thread_progress& t : x.threads) done += t.done;
      tracker.done_at_best = done;
    }
  }

  inline progress snapshot(const progress_tracker& tracker) {
    std::scoped_lock lock(tracker.mutex);

    progress p;
    p.total = tracker.total;
    p.elapsed_seconds
        = std::chrono::duration<double>(std::chrono::steady_clock::now() - tracker.start).count();

    std::uint64_t done = 0;
    double rate = 0.0;
    for (const rank_progress& rp : tracker.ranks) {
      rate += rp.rate;
      for (const thread_progress& t : rp.threads) done += t.done;
    }

    const std::uint64_t remaining = p.total > done ? p.total - done : 0;
    p.done = done;
    p.rate = rate;
    p.remaining_seconds
        = rate > 0.0 ? std::optional<double>{static_cast<double>(remaining) / rate} : std::nullopt;
    p.ranks = tracker.ranks;
    p.best = tracker.best;
    p.since_best = done - tracker.done_at_best;
    return p;
  }

  template <work_source W>
  std::optional<package> tracked_source<W>::next(const report& r, std::size_t thread) {
    record(tracker, rank, static_cast<std::uint32_t>(thread), r);
    return inner.next(r, thread);
  }

  inline std::size_t resolve_num_threads(const thread_config_t& threads,
                                         std::size_t ranks_on_node) {
    if (threads.has_value()) return std::max<std::size_t>(1, threads.value());
    const auto hardware = static_cast<std::size_t>(std::thread::hardware_concurrency());
    const std::size_t per_node = std::max<std::size_t>(1, ranks_on_node);
    return std::max<std::size_t>(1, hardware / per_node);
  }

  inline run_context make_local_run_context(const thread_config_t& threads, std::stop_token stop,
                                            progress_callback on_progress) {
    return run_context{
        .role = mpi_role::single,
        .num_threads = resolve_num_threads(threads, /*ranks_on_node=*/1),
        .num_remote_consumers = 0,
        .stop = std::move(stop),
        .on_progress = std::move(on_progress),
    };
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_RUNNER_IMPL_HPP
