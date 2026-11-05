#ifndef SQSGEN_OPTIMIZATION_SCHEDULER_H
#define SQSGEN_OPTIMIZATION_SCHEDULER_H

#include <cstdint>
#include <limits>
#include <optional>

// Hands out packages of the iteration range [0, total).
// Pure data: no threads, no MPI, no clock. The same scheduler is used
//   - without MPI: threads call next() directly (under a mutex, once per package)
//   - with MPI:    the head calls next() on behalf of the requesting rank

namespace sqsgen::optimization {

  // iterations [start, start + size)
  struct package {
    std::uint64_t start;
    std::uint64_t size;
  };

  // what a consumer (thread or rank) knows after its last package
  template <class T> struct report {
    T best{std::numeric_limits<T>::infinity()};
    std::optional<double> rate{std::nullopt};
    std::uint64_t done{0};
    bool stop{false};
  };

  struct scheduler_settings {
    std::uint64_t total;          // number of iterations, range [0, total)
    std::uint64_t block_size;     // B: package starts are multiples of B (reproducible seeds)
    std::uint64_t num_consumers;  // threads (x ranks) asking for work, used for the tail
    double target_seconds;        // tau: desired duration of one package
  };

  struct scheduler {
    scheduler_settings settings;
    std::uint64_t cursor{0};
    bool stopped{false};

    explicit scheduler(scheduler_settings s);

    template <class T> std::optional<package> next(const report<T>& r);

    void stop();

    bool finished() const;
  };

}  // namespace sqsgen::optimization

#include "sqsgen/impl/scheduler/scheduler_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_SCHEDULER_H
