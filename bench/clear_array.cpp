//
// Created by Dominik Gehringer on 05.11.24.
//

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include "sqsgen/core/structure.h"

namespace sqsgen::bench {

  template <class T, std::size_t Shell, std::size_t Species>
  void bench_fill(ankerl::nanobench::Bench* bench, char const* name) {
    std::array<T, Shell * Species * Species> a{};
    bench->run(std::format("{}-static", name), [&]() {
      std::fill(a.begin(), a.end(), 0);
      ankerl::nanobench::doNotOptimizeAway(a);
    });

    std::vector<T> v(Shell * Species * Species);
    bench->run(std::format("{}-dynamic", name), [&]() {
      std::fill(v.begin(), v.end(), 0);
      ankerl::nanobench::doNotOptimizeAway(v);
    });

    bench->run(std::format("{}-memset", name), [&]() {
      memset(v.data(), 0, v.size() * sizeof(T));
      ankerl::nanobench::doNotOptimizeAway(v);
    });
  }
}  // namespace sqsgen::bench

int main() {
  using namespace sqsgen::bench;
  ankerl::nanobench::Bench b73;
  b73.title("Clearing of static vs dynamic array (7, 3)")
      .warmup(1000)
      .minEpochIterations(2500000)
      .relative(true);
  b73.performanceCounters(true);

  bench_fill<std::size_t, 7, 3>(&b73, "std::size_t, 7, 3");
  bench_fill<std::uint_fast16_t, 7, 3>(&b73, "std::uint_fast16_t, 7, 3");
  bench_fill<std::uint_fast32_t, 7, 3>(&b73, "std::uint_fast32_t, 7, 3");

  ankerl::nanobench::Bench b53;
  b53.title("Clearing of static vs dynamic array (5, 3)")
      .warmup(1000)
      .minEpochIterations(2500000)
      .relative(true);
  b53.performanceCounters(true);
  bench_fill<std::size_t, 5, 3>(&b53, "std::size_t, 5, 3");
  bench_fill<std::uint_fast16_t, 5, 3>(&b53, "std::uint_fast16_t, 5, 3");
  bench_fill<std::uint_fast32_t, 5, 3>(&b53, "std::uint_fast32_t, 5, 3");
}