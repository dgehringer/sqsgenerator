//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_SRO_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_SRO_IMPL_HPP

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/optimization/sro.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  namespace helpers = sqsgen::core::helpers;

  template <class T>
  pair_list_t make_pair_list(const core::structure<T>& structure, const std::vector<T>& radii,
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

  template <class T>
  pair_objective_coefficients<T> pair_coefficients(const pair_tensor_t<T>& scaled_pair_weights,
                                                   const pair_tensor_t<T>& alpha_target,
                                                   const pair_tensor_t<T>& prefactors) {
    pair_tensor_t<T> a = scaled_pair_weights * (alpha_target - alpha_target.constant(T{1}));
    pair_tensor_t<T> b = scaled_pair_weights * prefactors;
    return {std::move(a), std::move(b)};
  }

  template <class T> pair_tensor_t<T> scaled_pair_weights(const pair_tensor_t<T>& pair_weights,
                                                          const std::map<shell_t, T>& weights) {
    const auto shell_to_index = helpers::reversed_index_map<shell_t>(weights | std::views::keys);

    pair_tensor_t<T> scaled = pair_weights;
    for (const auto& [shell, w_s] : weights) {
      const auto s = static_cast<Eigen::Index>(shell_to_index.at(shell));
      scaled.chip(s, 0) = scaled.chip(s, 0) * w_s;
    }
    return scaled;
  }

  inline void count_pairs(pair_tensor_t<count_t>& counts, const pair_list_t& pairs,
                          const configuration_t& species) {
    counts.setConstant(0);
    for (const pair& p : pairs) ++counts(p.shell, species[p.i], species[p.j]);
  }

  // Objective, canonical-cell summation; see compute_pair_objective in sro.h for the formula.
  template <class T> T compute_pair_objective(const pair_tensor_t<count_t>& bonds,
                                              const pair_objective_coefficients<T>& coeffs) {
    const Eigen::Index num_shells = bonds.dimension(0);
    const Eigen::Index num_species = bonds.dimension(1);

    T objective{0};
    for (Eigen::Index s = 0; s < num_shells; ++s)
      for (Eigen::Index xi = 0; xi < num_species; ++xi) {
        // diagonal: same species
        const T n_diag = static_cast<T>(bonds(s, xi, xi));
        objective += std::abs(coeffs.a(s, xi, xi) + n_diag * coeffs.b(s, xi, xi));

        // off-diagonal: sum both cells, since each bond landed in one of them
        for (Eigen::Index eta = xi + 1; eta < num_species; ++eta) {
          const T n = static_cast<T>(bonds(s, xi, eta) + bonds(s, eta, xi));
          objective += std::abs(coeffs.a(s, xi, eta) + n * coeffs.b(s, xi, eta));
        }
      }
    return objective;
  }

  // Builds the triangle list and class table; see make_triplet_list in sro.h for the contract.
  // For a triangle with sites (a, b, c), the shell "opposite" a is the shell of edge (b, c).
  template <class T> std::pair<triplet_list_t, triplet_class_mapping_t> make_triplet_list(
      const core::structure<T>& structure, const std::vector<T>& radii,
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

  // sites[0] is opposite the shortest edge, sites[2] opposite the longest;
  // see count_triplets in sro.h for the contract.
  inline void count_triplets(triplet_tensor_t<count_t>& counts, const triplet_list_t& triplets,
                             const configuration_t& species) {
    counts.setConstant(0);
    for (const triplet& t : triplets)
      ++counts(t.c, species[t.sites[0]], species[t.sites[1]], species[t.sites[2]]);
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_SRO_IMPL_HPP
