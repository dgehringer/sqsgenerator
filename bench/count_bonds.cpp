//
// Created by Dominik Gehringer on 06.11.24.
//
#include <nanobench.h>

#include <iostream>
#include <random>

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
                           std::vector<specie_t>& configuration, std::mt19937_64& rng,
                           std::size_t num_species) {
    std::shuffle(configuration.begin(), configuration.end(), rng);
    std::fill(bonds.begin(), bonds.end(), 0);
    auto num_params = num_species * num_species;
    for (auto const& [i, j, s] : pairs) {
      auto si{configuration[i]};
      auto sj{configuration[j]};
      ++bonds[s * num_params + sj * num_species + si];
      if (si != sj) ++bonds[s * num_params + si * num_species + sj];
    }
  }

  template<class T>
  T absolute(T a) {
    return a > 0 ? a : -a;
  }
  template <class T, class OutSize, class IndexSize, class ShellSize>
  void bench_count_bond(ankerl::nanobench::Bench* bench, const char* name) {
    auto supercell = sqsgen::bench::TEST_FCC_STRUCTURE<T, 2, 2, 2>;
    auto shell_weights = shell_weights_t<double>{{1, 1}, {2, 0.3}, {3, 1.0 / 3}};
    auto [pairs, map, rmap] = supercell.template pairs<IndexSize, ShellSize>(shell_weights);
    auto species = supercell.species;

    auto num_species = std::stoul(std::format("{}", core::count_species(species).size()));
    auto num_shells = std::stoul(std::format("{}", shell_weights.size()));
    auto num_params = num_species * num_species;

    std::cout << core::count_species(species).size() << ", " << num_species << std::endl;

    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::vector<OutSize> bonds(num_shells * num_params);

    bench->run(name, [&]() {
      std::shuffle(species.begin(), species.end(), rng);
      std::fill(bonds.begin(), bonds.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s * num_params + sj * num_species + si];
        if (si != sj) ++bonds[s * num_params + si * num_species + sj];
      }
    });


    // the idea is to sort by index difference to improve cache locality
    std::random_device rd2;
    std::mt19937_64 rng2(rd2());
    auto species2 = std::vector<specie_t>(species.size());
    auto pairs2 = std::vector{pairs};
    std::vector<OutSize> bonds2{bonds};
    std::sort(pairs2.begin(), pairs2.end(), [](auto p, auto q) {
      return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
    });

    bench->run(std::format("{}-sorted", name), [&]() {
      std::shuffle(species2.begin(), species2.end(), rng2);
      std::fill(bonds2.begin(), bonds2.end(), 0);
      for (auto& [i, j, s] : pairs2) {
        auto si{species2[i]};
        auto sj{species2[j]};
        ++bonds2[s * num_params + sj * num_species + si];
        if (si != sj) ++bonds2[s * num_params + si * num_species + sj];
      }
    });

    std::random_device rd3;
    std::mt19937_64 rng3(rd());
    std::vector<OutSize> bonds3{bonds};
    std::vector<IndexSize> species3(species.size());
    for (auto i = 0; i < species3.size(); i++) species3[i] = static_cast<IndexSize>(species[i]);
    bench->run(std::format("{}-configuration", name), [&]() {
      std::shuffle(species3.begin(), species3.end(), rng3);
      std::fill(bonds3.begin(), bonds3.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species3[i]};
        auto sj{species3[j]};
        ++bonds3[s * num_params + sj * num_species + si];
        if (si != sj) ++bonds3[s * num_params + si * num_species + sj];
      }
    });

    std::random_device rd4;
    std::mt19937_64 rng4(rd());
    std::vector<OutSize> bonds4(bonds);
    std::vector<IndexSize> species4(species.size());
    for (auto i = 0; i < species4.size(); i++) species4[i] = static_cast<IndexSize>(species[i]);
    bench->run(std::format("{}-half-off", name), [&]() {
      std::shuffle(species4.begin(), species4.end(), rng4);
      std::fill(bonds4.begin(), bonds4.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species4[i]};
        auto sj{species4[j]};
        ++bonds4[s * num_params + sj * num_species + si];
      }
    });

    // memory layout-dynamic
    std::random_device rd5;
    std::mt19937_64 rng5(rd());
    std::vector<std::vector<OutSize>> bonds5(num_shells);
    for (auto i = 0; i < num_shells; i++) bonds5[i].resize(num_params);
    auto species5 = std::vector<specie_t>(species.size());
    bench->run(std::format("{}-memory-layout", name), [&]() {
      std::shuffle(species5.begin(), species5.end(), rng5);
      for (auto i = 0; i < num_shells; i++) std::fill(bonds5[i].begin(), bonds5[i].end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species5[i]};
        auto sj{species5[j]};
        ++bonds5[s][si * num_species + sj];
      }
    });

    // memory layout static
    std::random_device rd6;
    std::mt19937_64 rng6(rd());
    std::array<OutSize, 3 * 3 * 3> bonds6{};
    std::vector<specie_t> species6(species);
    bench->run(std::format("{}-half-off-static", name), [&]() {
      std::shuffle(species6.begin(), species6.end(), rng6);
      std::fill(bonds6.begin(), bonds6.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species6[i]};
        auto sj{species6[j]};
        ++bonds6[s * 4 + sj * 2 + si];
      }
    });

    std::random_device rd7;
    std::mt19937_64 rng7(rd());
    std::array<OutSize, 3 * 3 * 3> bonds7{};
    std::vector<specie_t> species7(species);
    auto pairs7 = std::vector{pairs};
    std::sort(pairs7.begin(), pairs7.end(), [](auto p, auto q) {
      return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
    });

    bench->run(std::format("{}-half-off-static-sorted", name), [&]() {
      std::shuffle(species7.begin(), species7.end(), rng7);
      std::fill(bonds7.begin(), bonds7.end(), 0);
      for (auto& [i, j, s] : pairs7) {
        auto si{species7[i]};
        auto sj{species7[j]};
        ++bonds7[s * 4 + sj * 2 + si];
      }
    });

  }
}  // namespace sqsgen::bench

int main() {
  using namespace sqsgen;
  using namespace sqsgen::core;
  using namespace sqsgen::bench;

  ankerl::nanobench::Bench bcurr;
  bcurr.title("count bonds traditional").warmup(1000).minEpochIterations(200000).relative(true);
  bcurr.performanceCounters(true);
  bench_count_bond<double, std::size_t, std::size_t, std::size_t>(&bcurr, "current-std::size_t");
  bench_count_bond<double, std::uint_fast32_t, std::uint_fast32_t, std::uint_fast32_t>(&bcurr, "current-std::uint_fast32_t");
  bench_count_bond<double, std::uint_fast16_t, std::uint_fast16_t, std::uint_fast16_t>(&bcurr, "current-std::uint_fast16_t");

}