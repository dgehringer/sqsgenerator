#ifndef SQSGEN_IMPL_OPTIMIZATION_SCHEDULER_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_SCHEDULER_IMPL_HPP

#include <algorithm>
#include <cassert>

namespace sqsgen::optimization {

  namespace detail {

    // largest multiple of B that is <= x, but at least B
    constexpr std::uint64_t round_to_blocks(std::uint64_t x, std::uint64_t block_size) {
      return std::max(block_size, (x / block_size) * block_size);
    }

    // iterations that take about tau seconds at the given rate: rate * tau, capped at limit
    // (computed in double, so the cap avoids overflow when converting back)
    inline std::uint64_t size_from_rate(double rate, double tau, std::uint64_t limit) {
      const double wanted = rate * tau;
      if (!(wanted > 0.0)) return 0;
      if (wanted >= static_cast<double>(limit)) return limit;
      return static_cast<std::uint64_t>(wanted);
    }

  }  // namespace detail

  inline scheduler::scheduler(scheduler_settings s) : settings(s) {
    assert(settings.block_size > 0);
    assert(settings.num_consumers > 0);
    assert(settings.target_seconds > 0.0);
  }

  // Package size, step by step:
  //   1. from the rate:  n = rate * tau         (one block if no rate measured yet)
  //   2. ramp-up:        n <= 2 * done           (the first rates come from small, noisy packages)
  //   3. tail:           n <= ceil(R / (2 C))    (R remaining iterations, C consumers:
  //                                               near the end everyone gets a fair share)
  //   4. blocks:         n = multiple of B, at least B, at most R
  // Starts stay multiples of B, because every package except the last is a multiple of B.
  inline std::optional<package> scheduler::next(const report& r) {
    if (r.stop) stop();
    if (finished()) return std::nullopt;

    const std::uint64_t B = settings.block_size;
    const std::uint64_t remaining = settings.total - cursor;

    // 1. from the rate
    std::uint64_t size
        = r.rate > 0.0 ? detail::size_from_rate(r.rate, settings.target_seconds, remaining) : B;

    // 2. ramp-up: at most double the previous package
    if (r.done > 0)
      size = std::min(size, 2 * r.done);
    else
      size = B;

    // 3. tail: at most an equal share of half the remaining work
    const std::uint64_t share
        = (remaining + 2 * settings.num_consumers - 1) / (2 * settings.num_consumers);
    size = std::min(size, share);

    // 4. whole blocks, but never past the end
    size = std::min(detail::round_to_blocks(size, B), remaining);

    const package p{cursor, size};
    cursor += size;
    return p;
  }

  inline void scheduler::stop() { stopped = true; }

  inline bool scheduler::finished() const { return stopped || cursor >= settings.total; }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_SCHEDULER_IMPL_HPP
