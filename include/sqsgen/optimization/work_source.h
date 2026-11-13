#ifndef SQSGEN_OPTIMIZATION_WORK_SOURCE_H
#define SQSGEN_OPTIMIZATION_WORK_SOURCE_H

#include <atomic>
#include <concepts>
#include <cstddef>
#include <mutex>
#include <optional>

#include "sqsgen/optimization/scheduler.h"

/**
 * Executor, kernel and work source, independent of MPI.
 *   kernel:      processes one package, returns what it learned (report)
 *   work_source: turns a report into the next package (or nullopt = finished)
 *   execute:     one loop per thread:  report -> next package -> kernel -> report -> ...
 * Without MPI and on the MPI head, threads use local_source over a shared_scheduler.
 * On MPI worker ranks they use mpi::mpi_source (sqsgen/mpi/mpi_source.h).
 */

namespace sqsgen::optimization {

  template <class W>
  concept work_source = requires(W& w, const report& r) {
    { w.next(r) } -> std::same_as<std::optional<package>>;
  };

  /**
   * run(p, thread) processes iterations [p.start, p.start + p.size) with the scratch of
   * `thread`; it measures the rate and fills report{best, rate, done = p.size, stop}
   */
  template <class K>
  concept kernel = requires(K& k, const package& p, std::size_t thread) {
    { k.run(p, thread) } -> std::same_as<report>;
  };

  /**
   * the scheduler plus the lock that protects it; shared by local threads and (on the
   * MPI head) by the main thread answering remote requests
   */
  struct shared_scheduler {
    scheduler sched;
    std::mutex mutex;

    explicit shared_scheduler(scheduler_settings settings);
  };

  // thread-safe next(); called once per package, so the lock is not in the hot path
  std::optional<package> next_package(shared_scheduler& shared, const report& r);

  struct local_source {
    shared_scheduler& shared;

    std::optional<package> next(const report& r);
  };

  // value = min(value, candidate), lock-free
  template <class T> void atomic_min(std::atomic<T>& value, T candidate);

  // first report of every thread: nothing measured yet, nothing done
  report initial_report();

  /**
   * starts num_threads threads, each looping  next(report) -> run(package) -> report
   * returns when every thread got nullopt
   */
  template <work_source W, kernel K> void execute(W& source, K& k, std::size_t num_threads);

}  // namespace sqsgen::optimization

#include "sqsgen/impl/work_source/work_source_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_WORK_SOURCE_H
