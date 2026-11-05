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
  struct report {
    double best{std::numeric_limits<double>::infinity()};
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

    /**
     * @brief Hand out the next package of iterations, or nullopt once finished.
     *
     * The package size n is clamped in four steps:
     *   1. from the rate:  n = rate * tau        (one block B if no rate measured yet)
     *   2. ramp-up:        n <= 2 * done          (early rates come from small, noisy packages)
     *   3. tail:           n <= ceil(R / (2 C))   (R remaining, C consumers: a fair end share)
     *   4. blocks:         n = multiple of B, at least B, at most R
     * Package starts stay multiples of B, since every package except the last is a multiple of B.
     *
     * @param r  The reporting consumer's last result (rate, work done, stop flag).
     * @return The next package, or std::nullopt once the range is exhausted or stop() was called.
     */
    std::optional<package> next(const report& r);

    void stop();

    bool finished() const;
  };

}  // namespace sqsgen::optimization

#include "sqsgen/impl/scheduler/scheduler_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_SCHEDULER_H
