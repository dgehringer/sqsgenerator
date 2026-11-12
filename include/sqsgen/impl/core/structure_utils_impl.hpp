//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CORE_STRUCTURE_UTILS_IMPL_HPP
#define SQSGEN_IMPL_CORE_STRUCTURE_UTILS_IMPL_HPP

#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/core/structure_utils.h"
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

  template <class T> std::vector<T> distances_naive(const structure<T>& structure, T atol, T rtol) {
    using namespace sqsgen::core::helpers;
    sorted_vector<T> dists(structure.distance_matrix().reshaped());
    auto reduced = std::ranges::fold_left(dists, std::vector<T>{T(0)}, [&](auto&& vec, auto dist) {
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
    auto distances = structure.distance_matrix().reshaped()
                     | std::ranges::views::filter(
                         [](auto dist) { return dist > 0.0 && !is_close<T>(dist, 0.0); })
                     | ranges::to<std::vector>();
    std::sort(distances.begin(), distances.end());
    T min_dist{distances.front()}, max_dist{distances.back()};

    auto num_edges{static_cast<std::size_t>((max_dist - min_dist) / bin_width) + 2};

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
    assert(std::ranges::fold_left(
               std::ranges::views::elements<1>(freqs)
                   | std::ranges::views::transform([&](auto v) { return v.size(); }),
               std::size_t{0}, std::plus{})
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

  template <class T>
  pair_list_t make_pair_list(const structure<T>& structure, const std::vector<T>& radii,
                             const std::map<shell_t, T>& weights, T atol, T rtol) {
    const auto shell_to_index = helpers::reversed_index_map<shell_t>(weights | std::views::keys);

    const auto shell_matrix = structure.shell_matrix(radii, atol, rtol);
    const std::size_t num_sites = structure.size();

    pair_list_t pairs;
    for (std::size_t i = 0; i < num_sites; ++i)
      for (std::size_t j = i + 1; j < num_sites; ++j) {
        const auto it = shell_to_index.find(static_cast<shell_t>(shell_matrix(i, j)));
        if (it == shell_to_index.end()) continue;  // shell is skipped
        pairs.push_back(
            pair{.i = static_cast<site_t>(i), .j = static_cast<site_t>(j), .shell = it->second});
      }
    return pairs;
  }

  // Builds the triangle list and class table; see make_triplet_list in structure_utils.h for the
  // contract. For a triangle with sites (a, b, c), the shell "opposite" a is the shell of edge
  // (b, c).
  template <class T> std::pair<triplet_list_t, triplet_class_mapping_t> make_triplet_list(
      const structure<T>& structure, const std::vector<T>& radii,
      const std::map<shell_t, T>& weights, T atol, T rtol) {
    const auto shell_matrix = structure.shell_matrix(radii, atol, rtol);
    const std::size_t num_sites = structure.size();

    auto shell_of
        = [&](std::size_t a, std::size_t b) { return static_cast<shell_t>(shell_matrix(a, b)); };
    auto selected = [&](std::size_t a, std::size_t b) { return weights.contains(shell_of(a, b)); };

    // neighbours with larger index, connected by a selected shell
    std::vector<std::vector<site_t>> neighbours(num_sites);
    for (std::size_t i = 0; i < num_sites; ++i)
      for (std::size_t j = i + 1; j < num_sites; ++j)
        if (selected(i, j)) neighbours[i].push_back(static_cast<site_t>(j));

    // 1. collect all triangles with their sorted (opposite shell, site) pairs
    struct found_triangle {
      std::array<site_t, 3> sites;
      std::array<shell_t, 3> shells;
    };
    std::vector<found_triangle> found;

    for (std::size_t i = 0; i < num_sites; ++i)
      for (std::size_t n1 = 0; n1 < neighbours[i].size(); ++n1)
        for (std::size_t n2 = n1 + 1; n2 < neighbours[i].size(); ++n2) {
          const site_t j = neighbours[i][n1];
          const site_t k = neighbours[i][n2];  // i < j < k
          if (!selected(j, k)) continue;       // third edge must be selected too

          // (opposite shell, site): site i is opposite edge (j, k), etc.
          std::array<std::pair<shell_t, site_t>, 3> nodes{{
              {shell_of(j, k), static_cast<site_t>(i)},
              {shell_of(i, k), j},
              {shell_of(i, j), k},
          }};
          std::ranges::sort(nodes);  // by shell, then by site

          found.push_back({{nodes[0].second, nodes[1].second, nodes[2].second},
                           {nodes[0].first, nodes[1].first, nodes[2].first}});
        }

    // 2. number the occurring classes in sorted order (deterministic on every platform)
    std::map<std::array<shell_t, 3>, triplet_class_t> class_index;
    for (const auto& t : found) class_index.try_emplace(t.shells, 0);
    triplet_class_mapping_t classes;
    classes.reserve(class_index.size());
    for (auto& [shells, c] : class_index) {
      c = static_cast<triplet_class_t>(classes.size());
      classes.push_back(shells);
    }

    // 3. build the final list
    triplet_list_t triplets;
    triplets.reserve(found.size());
    for (const auto& t : found)
      triplets.push_back(triplet{.sites = t.sites, .c = class_index.at(t.shells)});

    return {std::move(triplets), std::move(classes)};
  }

}  // namespace sqsgen::core

#endif  // SQSGEN_IMPL_CORE_STRUCTURE_UTILS_IMPL_HPP
