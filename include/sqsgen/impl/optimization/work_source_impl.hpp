#ifndef SQSGEN_IMPL_OPTIMIZATION_WORK_SOURCE_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_WORK_SOURCE_IMPL_HPP

#include <limits>
#include <thread>
#include <vector>

#include "sqsgen/optimization/work_source.h"

namespace sqsgen::optimization {

  inline shared_scheduler::shared_scheduler(scheduler_settings settings) : sched(settings) {}

  inline std::optional<package> next_package(shared_scheduler& shared, const report& r) {
    std::scoped_lock lock(shared.mutex);
    return shared.sched.next(r);
  }

  // r.best is not needed here: local threads share the gate as an atomic directly
  inline std::optional<package> local_source::next(const report& r) {
    return next_package(shared, r);
  }

  template <class T> void atomic_min(std::atomic<T>& value, T candidate) {
    T current = value.load(std::memory_order_relaxed);
    // on failure, compare_exchange reloads current; stop once current <= candidate
    while (candidate < current
           && !value.compare_exchange_weak(current, candidate, std::memory_order_relaxed)) {
    }
  }

  inline report initial_report() {
    return report{
        .best = std::numeric_limits<double>::infinity(),
        .rate = 0.0,
        .done = 0,
        .stop = false,
    };
  }

  template <work_source W, kernel K> void execute(W& source, K& k, std::size_t num_threads) {
    std::vector<std::jthread> threads;
    threads.reserve(num_threads);
    for (std::size_t t = 0; t < num_threads; ++t)
      threads.emplace_back([&source, &k, t] {
        report r = initial_report();
        while (const auto p = source.next(r)) r = k.run(*p, t);
      });
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_WORK_SOURCE_IMPL_HPP
