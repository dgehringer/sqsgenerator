//
// Created by Dominik Gehringer on 05.11.24.
//

#define ANKERL_NANOBENCH_IMPLEMENT
#include "core/shuffle.h"

#include <nanobench.h>

#include "sqsgen/core/structure.h"

namespace sqsgen::bench {

  template <class T, class As = T> std::string format_vector(std::vector<T> const& values) {
    if (values.empty()) return "{}";
    if (values.size() == 1) {
      return std::format("{{{}}}", static_cast<As>(values.front()));
    }
    std::stringstream ss;
    ss << "{";
    for (auto i = 0; i < values.size() - 1; i++) ss << static_cast<As>(values.at(i)) << ", ";
    ss << static_cast<As>(values.back()) << "}";
    return ss.str();
  }

  void shuffle_configuration(configuration_t &configuration, uint64_t &seed) {
    for (auto i=static_cast<uint32_t>(configuration.size()); i > 1; i--) {
      uint32_t p = core::random_bounded(i, seed); // number in [0,i)
      std::swap(configuration[i-1], configuration[p]); // swap the values at i-1 and p
    }
  }

  template <class Random> void bench_shuffle(ankerl::nanobench::Bench* bench, char const* name) {
    std::vector<specie_t> species(64);
    std::iota(species.begin(), species.end(), 0);

    std::random_device rd;
    Random rng(rd());
    bench->run(name, [&]() {
      std::shuffle(species.begin(), species.end(), rng);
      ankerl::nanobench::doNotOptimizeAway(species);
    });
  }
}  // namespace sqsgen::bench

int main() {
  using namespace sqsgen::bench;
  ankerl::nanobench::Bench b;
  b.title("Clearing of static vs dynamic array (7, 3)")
      .warmup(1000)
      .minEpochIterations(100000)
      .relative(true);
  b.performanceCounters(true);



  bench_shuffle<std::mt19937>(&b, "mt19937");
  bench_shuffle<std::mt19937_64>(&b, "mt19937_64");
  //bench_shuffle<std::ranlux24>(&b, "ranlux24");
  bench_shuffle<std::knuth_b>(&b, "knuth_b");

  sqsgen::configuration_t species(64);
  std::iota(species.begin(), species.end(), 0);
  uint64_t seed = 0xbdd89aa982704029ull;
  b.run("rapidhash", [&]() {
      shuffle_configuration(species, seed);
      ankerl::nanobench::doNotOptimizeAway(species);
    });

  std::cout << format_vector<sqsgen::specie_t, int>(species) << std::endl;
}