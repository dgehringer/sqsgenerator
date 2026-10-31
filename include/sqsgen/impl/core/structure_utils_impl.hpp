//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef SQSGEN_IMPL_CORE_STRUCTURE_UTILS_IMPL_HPP
#define SQSGEN_IMPL_CORE_STRUCTURE_UTILS_IMPL_HPP

#include <Eigen/Dense>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <format>
#include <limits>
#include <map>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include "sqsgen/core/eigen.h"
#include "sqsgen/core/helpers/count.h"
#include "sqsgen/core/helpers/fold.h"
#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/permutation.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/core/structure_utils.h"
#include "sqsgen/log.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  template <class T>
  matrix_t<T> distance_matrix(const lattice_t<T>& lattice, const coords_t<T>& frac_coords) {
    const coords_t<T> cart_coords = frac_coords * lattice;
    assert(frac_coords.cols() == 3);
    const auto num_atoms = frac_coords.rows();

    auto a{lattice.row(0)};
    auto b{lattice.row(1)};
    auto c{lattice.row(2)};

    matrix_t<T> distances = matrix_t<T>::Ones(num_atoms, num_atoms) * std::numeric_limits<T>::max();
#pragma omp parallel for schedule(static) shared(distances) \
    firstprivate(a, b, c, num_atoms, cart_coords) if (num_atoms > 100)
    for (auto i = 0; i < num_atoms; i++) {
      auto p1{cart_coords.row(i)};
      for (auto j = i; j < num_atoms; j++) {
        auto p2{cart_coords.row(j)};
        for (auto u = -1; u <= 1; ++u)
          for (auto v = -1; v <= 1; ++v)
            for (auto w = -1; w <= 1; ++w) {
              auto t = u * a + v * b + w * c;
              auto diff = p1 - (t + p2);
              T image_norm{std::abs(diff.norm())};
              if (image_norm < distances(i, j)) {
                distances(i, j) = image_norm;
                if (i != j) distances(j, i) = image_norm;
                assert(distances(i, j) == distances(j, i));
              }
            }
      }
    }
    assert(distances.rows() == num_atoms && distances.cols() == num_atoms);
    return distances;
  }

  template <class T> shell_matrix_t shell_matrix(const matrix_t<T>& distance_matrix,
                                                 const std::vector<T>& dists, T atol, T rtol) {
    assert(distance_matrix.rows() == distance_matrix.cols());
    const auto num_atoms{distance_matrix.rows()};

    auto is_close_tol = [=](T a, T b) { return helpers::is_close(a, b, atol, rtol); };

    auto find_shell = [&](T d) {
      if (d < 0) throw std::out_of_range("Invalid distance matrix input");
      if (is_close_tol(d, 0.0)) return 0;

      for (auto i = 0; std::cmp_less(i, dists.size() - 1); i++) {
        T lb{dists[i]}, up{dists[i + 1]};
        if ((is_close_tol(d, lb) or d > lb) and (is_close_tol(d, up) or up > d)) {
          return i + 1;
        }
      }
      return static_cast<int>(dists.size());
    };
    shell_matrix_t shells = matrix_t<std::size_t>(num_atoms, num_atoms);
    for (auto i = 0; i < num_atoms; ++i) {
      for (auto j = i + 1; j < num_atoms; ++j) {
        auto shell{find_shell(distance_matrix(i, j))};
        shells(i, j) = static_cast<std::size_t>(shell);
        shells(j, i) = static_cast<std::size_t>(shell);
      }
    }
    for (auto i = 0; i < num_atoms; ++i) shells(i, i) = 0;
    return shells;
  }

  template <class T> structure<T> supercell(const structure<T>& structure, std::size_t a,
                                            std::size_t b, std::size_t c) {
    auto num_copies = a * b * c;
    if (num_copies == 0)
      throw std::invalid_argument("There must be at least one copy in each direction");
    lattice_t<T> scale = lattice_t<T>::Zero();
    scale(0, 0) = a;
    scale(1, 1) = b;
    scale(2, 2) = c;

    auto site_index{0};
    auto num_atoms{structure.species.size()};
    coords_t<T> supercell_coords(num_atoms * num_copies, 3);
    lattice_t<T> iscale = scale.inverse();
    coords_t<T> scaled_frac_coords = structure.frac_coords * iscale.transpose();
    std::vector<specie_t> supercell_species(num_atoms * num_copies);
    for (std::size_t i = 0; i < a; ++i)
      for (std::size_t j = 0; j < b; ++j)
        for (std::size_t k = 0; k < c; ++k) {
          using vec3_t = Eigen::Matrix<T, 1, 3>;
          vec3_t translation
              = vec3_t{static_cast<T>(i), static_cast<T>(j), static_cast<T>(k)} * iscale;
          for (std::size_t index = 0; index < num_atoms; ++index) {
            supercell_coords.row(site_index) = translation + scaled_frac_coords.row(index);
            supercell_species[site_index] = structure.species[index];
            site_index++;
          }
        }

    return core::structure<T>(structure.lattice * scale, supercell_coords, supercell_species,
                              structure.pbc);
  }

  template <class T> cube_t<T> compute_prefactors(const shell_matrix_t& shell_matrix,
                                                  const std::map<std::size_t, T>& weights,
                                                  const configuration_t& configuration) {
    using namespace helpers;
    if (weights.empty()) throw std::out_of_range("no coordination shells selected");
    auto neighbors = count(shell_matrix.reshaped());
    for (const auto& [shell, count] : neighbors) {
      auto atoms_per_shell{static_cast<T>(count) / static_cast<T>(configuration.size())};
      if (atoms_per_shell < 1)
        log::warn(std::format(
            R"(The coordination shell {} contains no or only one lattice position(s). Increase either "atol" or "rtol" or set the "shell_radii" parameter manually)",
            shell));
      if (!is_close(atoms_per_shell, static_cast<T>(static_cast<std::size_t>(atoms_per_shell))))
        log::warn(std::format(
            "The coordination shell {} does not contain an integer number of sites. I hope you "
            "know what you are doing",
            count));
      neighbors[shell] = atoms_per_shell;
    }

    auto shell_map = index_map<std::size_t>(weights | views::elements<0>);
    auto conf_map = index_map<std::size_t>(configuration);

    auto hist = core::count_species(configuration);
    auto num_sites{configuration.size()};
    auto num_species{static_cast<long>(hist.size())};
    auto num_shells{weights.size()};

    Eigen::Tensor<T, 3> prefactors(num_shells, num_species, num_species);

    for (std::size_t s = 0; s < num_shells; s++) {
      const auto neighbor_count = neighbors[shell_map[s]];
      if (neighbor_count > 0) {
        T M_i{static_cast<T>(neighbor_count)};
        for (auto a = 0; a < num_species; a++) {
          T x_a{static_cast<T>(hist[conf_map[a]]) / static_cast<T>(num_sites)};
          for (auto b = a; b < num_species; b++) {
            T x_b{static_cast<T>(hist[conf_map[b]]) / static_cast<T>(num_sites)};
            T prefactor{T(1.0) / (M_i * x_a * x_b * static_cast<T>(num_sites))};
            prefactors(s, a, b) = prefactor;
            prefactors(s, b, a) = prefactor;
          }
        }
      } else {
        log::warn(std::format(
            "The coordination shell {} contains no sites. This shell will not contribute to the "
            "objective function and can be removed from the \"shell_weights\" parameter",
            shell_map[s]));
        for (auto a = 0; a < num_species; a++)
          for (auto b = a; b < num_species; b++) {
            prefactors(s, a, b) = 0;
            prefactors(s, b, a) = 0;
          }
      }
    }
    return prefactors;
  }

  template <class T>
  cube_t<T> compute_prefactors(const structure<T>& structure, const std::vector<T>& shell_radii,
                               const std::map<std::size_t, T>& weights, T atol, T rtol) {
    return compute_prefactors(shell_matrix<T>(structure.distance_matrix(), shell_radii, atol, rtol),
                              weights, structure.species);
  }

  template <class T> std::vector<T> distances_naive(const structure<T>& structure, T atol, T rtol) {
    using namespace sqsgen::core::helpers;
    sorted_vector<T> dists(structure.distance_matrix().reshaped());
    auto reduced = fold_left(dists, std::vector<T>{T(0)}, [&](auto&& vec, auto dist) {
      if (is_close(vec.back(), dist, atol, rtol))
        vec[vec.size() - 1] = 0.5 * (dist + vec.back());
      else
        vec.push_back(dist);
      return vec;
    });
    return reduced;
  }

  template <class T>
  std::vector<T> distances_histogram(const structure<T>& structure, T bin_width, T peak_isolation) {
    using namespace helpers;
    auto distances
        = structure.distance_matrix().reshaped()
          | views::filter([](auto dist) { return dist > 0.0 && !is_close<T>(dist, 0.0); })
          | ranges::to<std::vector>();
    std::sort(distances.begin(), distances.end());
    T min_dist{distances.front()}, max_dist{distances.back()};

    auto num_edges{static_cast<std::size_t>((max_dist - min_dist) / bin_width) + 2};

    // for some reason ranges::to<std::vector> does not work here
    auto edges = std::vector<T>(num_edges);
    for (std::size_t i = 0; i < num_edges; i++) edges[i] = min_dist + static_cast<T>(i) * bin_width;

    if (edges.size() < 10)
      throw std::invalid_argument(
          "Not enough edges to create a histogram, please increase the bin width");
    auto freqs = std::map<std::size_t, std::vector<T>>{{0, {}}};
    std::size_t index{0}, bin{0};
    while (index < distances.size() && bin < num_edges - 1) {
      T lower{edges[bin]}, upper{edges[bin + 1]}, value{distances[index]};
      if (lower <= value && value < upper) {
        freqs[bin].push_back(value);
        ++index;
      } else
        freqs[++bin] = std::vector<T>{};
    }
    // make sure our histogramm contains the same number of distances as the corresponding input
    // vector
    assert(fold_left(views::elements<1>(freqs) | views::transform([&](auto v) { return v.size(); }),
                     0, std::plus{})
           == distances.size());
    const auto get_freq
        = [&](auto i) { return freqs.contains(i) ? freqs.at(i) : std::vector<T>{}; };
    assert(freqs.size() == num_edges - 1);
    std::vector<T> shells;
    for (auto i = 0; std::cmp_less_equal(i, num_edges); i++) {
      auto prev{get_freq(i - 1)}, f{get_freq(i)}, next{get_freq(i + 1)};
      auto threshold = static_cast<std::size_t>((1.0 - peak_isolation) * static_cast<T>(f.size()));
      if (threshold > prev.size() && threshold > next.size()) {
        assert(freqs.size() > 0);
        shells.push_back(*std::max_element(f.begin(), f.end()));
      }
    }

    if (shells.front() != 0.0 && !is_close<T>(shells.front(), 0.0))
      shells.insert(shells.begin(), 0.0);
    return shells;
  }

}  // namespace sqsgen::core

#endif  // SQSGEN_IMPL_CORE_STRUCTURE_UTILS_IMPL_HPP
