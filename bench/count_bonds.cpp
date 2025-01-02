//
// Created by Dominik Gehringer on 06.11.24.
//

#include <nanobench.h>

#include <fstream>
#include <iostream>
#include <random>

#include "sqsgen/core/shuffle.h"
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

  template <class T, std::size_t A = 2, std::size_t B = 2, std::size_t C = 3>
  auto make_structure() {
    return core::structure<T>(
               lattice_t<T>{{4.05, 0.0, 0.0}, {0.0, 4.05, 0.0}, {0.0, 0.0, 4.05}},
               coords_t<T>{{0.0, 0.0, 0.0}, {0.5, 0.5, 0.0}, {0.5, 0.0, 0.5}, {0.0, 0.5, 0.5}},
               {1, 2, 1, 3})
        .supercell(A, B, C);
  }

  template <class OutSize, class Size>
  void count_bonds_current(std::vector<OutSize>& bonds,
                           std::vector<core::atom_pair<Size>> const& pairs,
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

  template <class T> T absolute(T a) { return a > 0 ? a : -a; }

  auto prepare_test_data() {
    auto supercell
        = core::structure<double>(
              lattice_t<double>{{4.05, 0.0, 0.0}, {0.0, 4.05, 0.0}, {0.0, 0.0, 4.05}},
              coords_t<double>{{0.0, 0.0, 0.0}, {0.5, 0.5, 0.0}, {0.5, 0.0, 0.5}, {0.0, 0.5, 0.5}},
              {1, 2, 1, 3})
              .supercell(2, 2, 3);
    auto shell_weights = shell_weights_t<double>{{1, 1}, {2, 0.3}, {3, 1.0 / 3}, {4, 0.25}};
    auto radii = core::distances_naive(std::forward<core::structure<double>>(supercell));
    auto [pairs, map, rmap] = supercell.template pairs<usize_t>(radii, shell_weights);
    auto species = supercell.packed_species();

    auto num_species = std::stoul(std::format("{}", core::count_species(species).size()));
    auto num_shells = std::stoul(std::format("{}", shell_weights.size()));
    auto num_params = num_species * num_species;
    /*std::cout << std::format("Nspecies={}, Nshells={}, Npairs={}", num_species, num_shells,
                             pairs.size())
              << std::endl;

    std::cout << supercell.shell_matrix() << std::endl;*/

    return std::make_tuple(pairs, species, num_species, num_shells, num_params);
  }

  void bench_count_bond_current(ankerl::nanobench::Bench* bench) {
    auto [pairs, species, num_species, num_shells, num_params] = prepare_test_data();
    core::shuffler shuffler{};
    std::vector<usize_t> bonds(num_shells * num_params);

    bench->run("current", [&]() {
      shuffler.shuffle(species);
      std::fill(bonds.begin(), bonds.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s * num_params + sj * num_species + si];
        if (si != sj) ++bonds[s * num_params + si * num_species + sj];
      }
    });
  }

  void bench_count_bond_current_sorted(ankerl::nanobench::Bench* bench) {
    auto [pairs, species, num_species, num_shells, num_params] = prepare_test_data();
    core::shuffler shuffler{};
    std::vector<usize_t> bonds(num_shells * num_params);
    std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
      return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
    });
    bench->run("current-sorted", [&]() {
      shuffler.shuffle(species);
      std::fill(bonds.begin(), bonds.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s * num_params + sj * num_species + si];
        if (si != sj) ++bonds[s * num_params + si * num_species + sj];
      }
    });
  }

  void bench_count_bond_current_species_size_type(ankerl::nanobench::Bench* bench) {
    auto [pairs, species_, num_species, num_shells, num_params] = prepare_test_data();
    core::shuffler shuffler{};
    std::vector<usize_t> bonds(num_shells * num_params);
    std::vector<usize_t> species(species_.size());
    for (auto i = 0; i < species.size(); i++) species[i] = static_cast<usize_t>(species_[i]);
    bench->run("current-no-size-pad", [&]() {
      shuffler.shuffle(species);
      std::fill(bonds.begin(), bonds.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s * num_params + sj * num_species + si];
        if (si != sj) ++bonds[s * num_params + si * num_species + sj];
      }
    });
  }

  void bench_count_bond_half_off(ankerl::nanobench::Bench* bench) {
    auto [pairs, species, num_species, num_shells, num_params] = prepare_test_data();
    core::shuffler shuffler{};
    std::vector<usize_t> bonds(num_shells * num_params);
    bench->run("draft-half-off", [&]() {
      shuffler.shuffle(species);
      std::fill(bonds.begin(), bonds.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s * num_params + sj * num_species + si];
      }
    });
  }

  void bench_count_bond_half_off_sorted(ankerl::nanobench::Bench* bench) {
    auto [pairs, species, num_species, num_shells, num_params] = prepare_test_data();
    core::shuffler shuffler{};
    std::vector<usize_t> bonds(num_shells * num_params);
    std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
      return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
    });
    bench->run("draft-half-off-sorted", [&]() {
      shuffler.shuffle(species);
      std::fill(bonds.begin(), bonds.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s * num_params + sj * num_species + si];
      }
    });
  }

  void bench_count_bond_half_off_sorted_memset(ankerl::nanobench::Bench* bench) {
    auto [pairs, species, num_species, num_shells, num_params] = prepare_test_data();
    core::shuffler shuffler{};
    std::vector<usize_t> bonds(num_shells * num_params);
    std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
      return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
    });
    bench->run("draft-half-off-sorted-memset", [&]() {
      shuffler.shuffle(species);
      memset(bonds.data(), 0, bonds.size() * sizeof(usize_t));
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s * num_params + sj * num_species + si];
      }
    });
  }

  void bench_count_bond_half_off_sorted_shells(ankerl::nanobench::Bench* bench) {
    auto [pairs, species, num_species, num_shells, num_params] = prepare_test_data();
    core::shuffler shuffler{};
    std::vector<usize_t> bonds(num_shells * num_params);
    std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
      return p.shell > q.shell;
    });
    bench->run("draft-half-off-sorted-shells", [&]() {
      shuffler.shuffle(species);
      std::fill(bonds.begin(), bonds.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s * num_params + sj * num_species + si];
      }
    });
  }

  void bench_count_bond_half_off_sorted_static(ankerl::nanobench::Bench* bench) {
    auto [pairs, species, num_species, num_shells, num_params] = prepare_test_data();
    core::shuffler shuffler{};
    if (num_shells != 4) throw std::invalid_argument("num_shells must be 4");
    if (num_params != 9) throw std::invalid_argument("num_params must be 4");
    if (num_species != 3) throw std::invalid_argument("num_params must be 3");
    std::array<usize_t, 4 * 9> bonds{};
    std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
      return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
    });
    bench->run("draft-half-off-sorted-static", [&]() {
      shuffler.shuffle(species);
      std::fill(bonds.begin(), bonds.end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s * 9 + sj * 3 + si];
      }
    });
  }

  void bench_count_bond_half_off_sorted_static_memory_layout(ankerl::nanobench::Bench* bench) {
    auto [pairs, species, num_species, num_shells, num_params] = prepare_test_data();
    core::shuffler shuffler{};
    if (num_shells != 4) throw std::invalid_argument("num_shells must be 4");
    if (num_params != 9) throw std::invalid_argument("num_params must be 4");
    if (num_species != 3) throw std::invalid_argument("num_params must be 3");
    std::array<std::array<usize_t, 9>, 4> bonds{};
    std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
      return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
    });
    bench->run("draft-half-off-sorted-static-memory-hierarchy", [&]() {
      shuffler.shuffle(species);
      for (auto i = 0; i < 4; i++) std::fill(bonds[i].begin(), bonds[i].end(), 0);
      for (auto& [i, j, s] : pairs) {
        auto si{species[i]};
        auto sj{species[j]};
        ++bonds[s][sj * 3 + si];
      }
    });
  }


  void gen(std::string const& typeName, char const* mustacheTemplate,
           ankerl::nanobench::Bench const& bench) {
    std::ofstream templateOut("mustache.template." + typeName);
    templateOut << mustacheTemplate;

    std::ofstream renderOut("mustache.render." + typeName);
    ankerl::nanobench::render(mustacheTemplate, bench, renderOut);
  }
}  // namespace sqsgen::bench
// namespace sqsgen::bench

int main() {
  using namespace sqsgen;
  using namespace sqsgen::core;
  using namespace sqsgen::bench;

  ankerl::nanobench::Bench bcurr;
  bcurr.title("count bonds traditional").warmup(1000).minEpochIterations(150000).relative(true);
  bcurr.performanceCounters(true);
  bench_count_bond_current(&bcurr);
  bench_count_bond_current_sorted(&bcurr);
  bench_count_bond_current_species_size_type(&bcurr);
  bench_count_bond_half_off(&bcurr);
  bench_count_bond_half_off_sorted(&bcurr);
  bench_count_bond_half_off_sorted_memset(&bcurr);
  bench_count_bond_half_off_sorted_shells(&bcurr);
  bench_count_bond_half_off_sorted_static(&bcurr);
  bench_count_bond_half_off_sorted_static_memory_layout(&bcurr);

  gen("json", ankerl::nanobench::templates::json(), bcurr);
  gen("csv", ankerl::nanobench::templates::csv(), bcurr);
}