//
// Created by Dominik Gehringer on 06.11.24.
//
#include <__random/random_device.h>
#include <nanobench.h>
#include <test/main.h>

#include <iostream>
#include <random>

#include "sqsgen/core/permutation.h"
#include "sqsgen/core/structure.h"

namespace sqsgen::bench {
  template <class T> std::string format_vector(std::vector<T> const& values) {
    if (values.empty()) return "{}";
    if (values.size() == 1) {
      return std::format("{{{}}}", values.front());
    }
    std::stringstream ss;
    ss << "{";
    for (auto i = 0; i < values.size() - 1; i++) ss << values.at(i) << ", ";
    ss << values.back() << "}";
    return ss.str();
  }

  template <class T, std::size_t A = 2, std::size_t B = 2, std::size_t C = 2>
  static auto TEST_FCC_STRUCTURE
      = core::structure<T>(
            lattice_t<T>{{4.05, 0.0, 0.0}, {0.0, 4.05, 0.0}, {0.0, 0.0, 4.05}},
            coords_t<T>{{0.0, 0.0, 0.0}, {0.5, 0.5, 0.0}, {0.5, 0.0, 0.5}, {0.0, 0.5, 0.5}},
            {1, 2, 1, 2})
            .supercell(A, B, C);

  template <class OutSize, class IndexSize, class ShellSize>
  void count_bonds_current(std::vector<OutSize>& bonds,
                           std::vector<core::atom_pair<IndexSize, ShellSize>> const& pairs,
                           std::vector<specie_t> &configuration, std::mt19937_64 &rng, auto num_species) {
    std::shuffle(pairs.begin(), pairs.end(), rng);
    std::fill(bonds.begin(), bonds.end(), 0);
    auto num_params = num_species * num_species;
    for (auto const& [i, j, s] : pairs) {
      auto si {configuration[i]};
      auto sj {configuration[j]};
      ++bonds[s * num_params + sj * num_species + si];
      if (si != sj) ++bonds[s * num_params + si * num_species + sj];
    }
  }

  template <class T, class OutSize, class IndexSize, class ShellSize, template <class...> class Fn>
  void bench_count_bond(ankerl::nanobench::Bench* bench, const char* name, Fn<OutSize, IndexSize, ShellSize>&& fn) {
    auto supercell = sqsgen::bench::TEST_FCC_STRUCTURE<T, 2, 2, 2>;
    auto shell_weights = shell_weights_t<double>{{1, 1}, {2, 0.3}, {3, 1.0 / 3}};
    auto pairs = std::get<0>(
      supercell.template pairs<IndexSize, ShellSize>(shell_weights));
    auto species = std::vector(supercell.packed_species());

    auto num_species = std::stoul(std::format("{}", core::count_species(species).size()));
    auto num_shells = std::stoul(std::format("{}", shell_weights.size()));
    auto num_params = num_species * num_species;



    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::vector<OutSize> bonds(num_shells * num_params);
    bench->run(name, [&]() {
      fn(bonds, pairs, species, rng, num_species);
    });
  }
}  // namespace sqsgen::bench

int main() {
  using namespace sqsgen;
  using namespace sqsgen::core;
  using namespace sqsgen::bench;

  ankerl::nanobench::Bench bcurr;
  bcurr.title("count bonds traditional")
      .warmup(1000)
      .minEpochIterations(150000)
      .relative(true);
  bcurr.performanceCounters(true);

}