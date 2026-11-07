//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_PREFACTORS_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_PREFACTORS_IMPL_HPP

#include "sqsgen/core/helpers/count.h"
#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/log.h"
#include "sqsgen/optimization/prefactors.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  namespace helpers = sqsgen::core::helpers;

  namespace detail {

    // x_xi = (number of sites with species xi) / N, indexed by packed species
    template <class T> std::vector<T> mole_fractions(const configuration_t& configuration) {
      auto hist = core::count_species(configuration);
      auto conf_map = helpers::index_map<std::size_t>(configuration);  // packed index -> species
      const auto num_sites = static_cast<T>(configuration.size());
      std::vector<T> x(hist.size());
      for (std::size_t a = 0; a < x.size(); ++a)
        x[a] = static_cast<T>(hist[conf_map[a]]) / num_sites;
      return x;
    }

    // Coordination number M_i of each selected shell:
    //   M_i = (number of shell matrix entries equal to i) / N
    // The shell matrix is symmetric, so each pair appears twice: #entries = N M_i.
    template <class T>
    std::map<shell_t, T> coordination_numbers(const shell_matrix_t& shell_matrix,
                                              const std::map<shell_t, T>& weights,
                                              std::size_t num_sites) {
      using namespace helpers;
      const auto entries = count(shell_matrix.reshaped());
      std::map<shell_t, T> coordination;
      for (const shell_t shell : weights | std::views::keys) {
        const auto it = entries.find(shell);
        const T M_i
            = it == entries.end() ? T{0} : static_cast<T>(it->second) / static_cast<T>(num_sites);
        if (M_i == T{0})
          log::warn(std::format(
              "The coordination shell {} contains no sites. This shell will not contribute to "
              "the objective function and can be removed from the \"pair_weights\" parameter",
              shell));
        else if (M_i < T{1})
          log::warn(std::format(
              R"(The coordination shell {} contains no or only one lattice position(s). Increase either "atol" or "rtol" or set the "shell_radii" parameter manually)",
              shell));
        else if (!is_close(M_i, std::round(M_i)))
          log::warn(std::format(
              "The coordination shell {} does not contain an integer number of sites. I hope "
              "you know what you are doing",
              shell));
        coordination.emplace(shell, M_i);
      }
      return coordination;
    }

    // m_{xi eta} = 2 - delta_{xi eta}
    // (number of ways to put the species on the two ends of a pair)
    constexpr int pair_multiplicity(specie_t xi, specie_t eta) { return xi == eta ? 1 : 2; }

    // m_c^{xi eta zeta} = |Pi_c(xi, eta, zeta)|
    //   = product over groups of interchangeable positions of the number of
    //     distinct orderings of the species inside the group.
    // Positions 1, 2, 3 are ordered by opposite shell s1 <= s2 <= s3; positions with
    // equal opposite shell are interchangeable.
    constexpr int triplet_multiplicity(const std::array<shell_t, 3>& shells, specie_t xi,
                                       specie_t eta, specie_t zeta) {
      const bool equal_12 = shells[0] == shells[1];
      const bool equal_23 = shells[1] == shells[2];
      if (equal_12 && equal_23) {  // {iii}: one group {1,2,3}, m = 3! / prod n_s!
        if (xi == eta && eta == zeta) return 1;
        if (xi == eta || eta == zeta || xi == zeta) return 3;
        return 6;
      }
      if (equal_12) return xi == eta ? 1 : 2;    // {iij}: groups {1,2}, {3}
      if (equal_23) return eta == zeta ? 1 : 2;  // {ijj}: groups {1}, {2,3}
      return 1;                                  // {ijk}: groups {1}, {2}, {3}
    }

  }  // namespace detail

  // Pair prefactors; see compute_pair_prefactors in prefactors.h for the formula.
  // Shell index s is the compact index of the selected shells (same order as the pair list).
  template <class T>
  pair_tensor_t<T> compute_pair_prefactors(const shell_matrix_t& shell_matrix,
                                           const std::map<shell_t, T>& weights,
                                           const configuration_t& configuration) {
    using namespace helpers;
    if (weights.empty()) throw std::out_of_range("no coordination shells selected");

    const std::size_t num_sites = configuration.size();
    const auto coordination = detail::coordination_numbers(shell_matrix, weights, num_sites);
    const auto x = detail::mole_fractions<T>(configuration);
    const auto shell_map = index_map<std::size_t>(weights | std::views::keys);  // s -> shell

    const auto num_shells = static_cast<Eigen::Index>(weights.size());
    const auto num_species = static_cast<Eigen::Index>(x.size());
    const T N = static_cast<T>(num_sites);

    pair_tensor_t<T> prefactors(num_shells, num_species, num_species);
    prefactors.setConstant(T{0});

    for (Eigen::Index s = 0; s < num_shells; ++s) {
      const T M_i = coordination.at(static_cast<shell_t>(shell_map.at(s)));
      if (M_i == T{0}) continue;  // already warned
      for (Eigen::Index xi = 0; xi < num_species; ++xi)
        for (Eigen::Index eta = xi; eta < num_species; ++eta) {
          const T m = static_cast<T>(
              detail::pair_multiplicity(static_cast<specie_t>(xi), static_cast<specie_t>(eta)));
          const T f = T{2} / (N * M_i * m * x[xi] * x[eta]);
          prefactors(s, xi, eta) = f;
          prefactors(s, eta, xi) = f;
        }
    }
    return prefactors;
  }

  template <class T> pair_tensor_t<T> compute_pair_prefactors(const core::structure<T>& structure,
                                                              const std::vector<T>& shell_radii,
                                                              const std::map<shell_t, T>& weights,
                                                              T atol, T rtol) {
    return compute_pair_prefactors(
        shell_matrix<T>(structure.distance_matrix(), shell_radii, atol, rtol), weights,
        structure.species);
  }

  // Triplet prefactors; see compute_triplet_prefactors in prefactors.h for the formula.
  // m and the product of mole fractions are invariant under permutations inside a group,
  // so every member of Pi_c(xi, eta, zeta) gets the same value as the canonical tuple.
  template <class T>
  triplet_tensor_t<T> compute_triplet_prefactors(const triplet_list_t& triplets,
                                                 const triplet_class_mapping_t& classes,
                                                 const configuration_t& configuration) {
    // n_c (not in the hot path, so std::size_t)
    std::vector<std::size_t> triangles(classes.size(), 0);
    for (const triplet& t : triplets) ++triangles[t.c];

    const auto x = detail::mole_fractions<T>(configuration);
    const auto num_classes = static_cast<Eigen::Index>(classes.size());
    const auto num_species = static_cast<Eigen::Index>(x.size());

    triplet_tensor_t<T> prefactors(num_classes, num_species, num_species, num_species);

    for (Eigen::Index c = 0; c < num_classes; ++c) {
      // classes are created from found triangles, so every class has n_c >= 1
      assert(triangles[c] > 0);
      const T n_c = static_cast<T>(triangles[c]);
      for (Eigen::Index xi = 0; xi < num_species; ++xi)
        for (Eigen::Index eta = 0; eta < num_species; ++eta)
          for (Eigen::Index zeta = 0; zeta < num_species; ++zeta) {
            const T m = static_cast<T>(detail::triplet_multiplicity(
                classes[c], static_cast<specie_t>(xi), static_cast<specie_t>(eta),
                static_cast<specie_t>(zeta)));
            prefactors(c, xi, eta, zeta) = T{1} / (n_c * m * x[xi] * x[eta] * x[zeta]);
          }
    }
    return prefactors;
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_PREFACTORS_IMPL_HPP
