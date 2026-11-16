//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_OBJECTIVE_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_OBJECTIVE_IMPL_HPP

#include <cmath>
#include <cstdint>

#include "sqsgen/optimization/objective.h"

namespace sqsgen::optimization {

  namespace detail {

    // Calls fn(s, xi, eta, Nbar) once per canonical cell xi <= eta:
    //   Nbar = N(s, xi, xi)                    xi == eta
    //   Nbar = N(s, xi, eta) + N(s, eta, xi)   xi <  eta  (each bond landed in one of them)
    template <class Fn> void for_each_pair_term(const pair_tensor_t<count_t>& bonds, Fn&& fn) {
      const Eigen::Index num_shells = bonds.dimension(0);
      const Eigen::Index num_species = bonds.dimension(1);
      for (Eigen::Index s = 0; s < num_shells; ++s)
        for (Eigen::Index xi = 0; xi < num_species; ++xi) {
          fn(s, xi, xi, bonds(s, xi, xi));
          for (Eigen::Index eta = xi + 1; eta < num_species; ++eta)
            fn(s, xi, eta, bonds(s, xi, eta) + bonds(s, eta, xi));
        }
    }

    // Which corners of a class are interchangeable. Corners are ordered by their opposite
    // shell s1 <= s2 <= s3; corners with equal opposite shell are interchangeable.
    enum class triplet_symmetry : std::uint8_t {
      ijk,  // s1 < s2 < s3   groups {1}, {2}, {3}
      iij,  // s1 = s2 < s3   groups {1, 2}, {3}
      ijj,  // s1 < s2 = s3   groups {1}, {2, 3}
      iii,  // s1 = s2 = s3   group  {1, 2, 3}
    };

    inline triplet_symmetry symmetry_of(const shell_triple& shells) {
      const bool equal_12 = shells[0] == shells[1];
      const bool equal_23 = shells[1] == shells[2];
      if (equal_12 && equal_23) return triplet_symmetry::iii;
      if (equal_12) return triplet_symmetry::iij;
      if (equal_23) return triplet_symmetry::ijj;
      return triplet_symmetry::ijk;
    }

    // Nbar for the {iii} class: sum over the distinct orderings of (x, y, z), x <= y <= z
    inline count_t sum_over_orderings(const triplet_tensor_t<count_t>& n, Eigen::Index c,
                                      Eigen::Index x, Eigen::Index y, Eigen::Index z) {
      if (x == y && y == z) return n(c, x, x, x);                           // 1
      if (x == y) return n(c, x, x, z) + n(c, x, z, x) + n(c, z, x, x);     // 3
      if (y == z) return n(c, x, y, y) + n(c, y, x, y) + n(c, y, y, x);     // 3
      return n(c, x, y, z) + n(c, x, z, y) + n(c, y, x, z) + n(c, y, z, x)  // 6
             + n(c, z, x, y) + n(c, z, y, x);
    }

    // Calls fn(c, xi, eta, zeta, Nbar) once per canonical cell of every class:
    //   ijk: every (xi, eta, zeta)          Nbar = N(xi, eta, zeta)
    //   iij: xi <= eta                      Nbar = N(xi, eta, zeta) + N(eta, xi, zeta)  [xi < eta]
    //   ijj: eta <= zeta                    Nbar = N(xi, eta, zeta) + N(xi, zeta, eta)  [eta <
    //   zeta] iii: xi <= eta <= zeta              Nbar = sum over the distinct orderings
    template <class Fn> void for_each_triplet_term(const triplet_tensor_t<count_t>& n,
                                                   const core::triplet_class_mapping_t& classes,
                                                   Fn&& fn) {
      const auto num_classes = static_cast<Eigen::Index>(classes.size());
      const Eigen::Index K = n.dimension(1);
      for (Eigen::Index c = 0; c < num_classes; ++c) {
        switch (symmetry_of(classes[c])) {
          case triplet_symmetry::ijk:
            for (Eigen::Index xi = 0; xi < K; ++xi)
              for (Eigen::Index eta = 0; eta < K; ++eta)
                for (Eigen::Index zeta = 0; zeta < K; ++zeta)
                  fn(c, xi, eta, zeta, n(c, xi, eta, zeta));
            break;
          case triplet_symmetry::iij:
            for (Eigen::Index xi = 0; xi < K; ++xi)
              for (Eigen::Index eta = xi; eta < K; ++eta)
                for (Eigen::Index zeta = 0; zeta < K; ++zeta)
                  fn(c, xi, eta, zeta,
                     xi == eta ? n(c, xi, eta, zeta) : n(c, xi, eta, zeta) + n(c, eta, xi, zeta));
            break;
          case triplet_symmetry::ijj:
            for (Eigen::Index xi = 0; xi < K; ++xi)
              for (Eigen::Index eta = 0; eta < K; ++eta)
                for (Eigen::Index zeta = eta; zeta < K; ++zeta)
                  fn(c, xi, eta, zeta,
                     eta == zeta ? n(c, xi, eta, zeta) : n(c, xi, eta, zeta) + n(c, xi, zeta, eta));
            break;
          case triplet_symmetry::iii:
            for (Eigen::Index xi = 0; xi < K; ++xi)
              for (Eigen::Index eta = xi; eta < K; ++eta)
                for (Eigen::Index zeta = eta; zeta < K; ++zeta)
                  fn(c, xi, eta, zeta, sum_over_orderings(n, c, xi, eta, zeta));
            break;
        }
      }
    }

  }  // namespace detail

  template <class T> T compute_pair_sro_objective(const pair_tensor_t<count_t>& bonds,
                                                  const objective_terms<pair_tensor_t<T>>& terms) {
    T objective{0};
    detail::for_each_pair_term(
        bonds, [&](Eigen::Index s, Eigen::Index xi, Eigen::Index eta, count_t nbar) {
          objective += terms.s(s, xi, eta)
                       * std::abs(terms.A(s, xi, eta) + static_cast<T>(nbar) * terms.B(s, xi, eta));
        });
    return objective;
  }

  template <class T>
  T compute_triplet_sro_objective(const triplet_tensor_t<count_t>& counts,
                                  const objective_terms<triplet_tensor_t<T>>& terms,
                                  const core::triplet_class_mapping_t& classes) {
    T objective{0};
    detail::for_each_triplet_term(
        counts, classes,
        [&](Eigen::Index c, Eigen::Index xi, Eigen::Index eta, Eigen::Index zeta, count_t nbar) {
          objective += terms.s(c, xi, eta, zeta)
                       * std::abs(terms.A(c, xi, eta, zeta)
                                  + static_cast<T>(nbar) * terms.B(c, xi, eta, zeta));
        });
    return objective;
  }

  template <class T>
  T compute_pair_energy_objective(const pair_tensor_t<count_t>& bonds,
                                  const objective_terms<pair_tensor_t<T>>& terms) {
    T energy{0};
    detail::for_each_pair_term(
        bonds, [&](Eigen::Index s, Eigen::Index xi, Eigen::Index eta, count_t nbar) {
          energy += terms.s(s, xi, eta) * terms.B(s, xi, eta) * static_cast<T>(nbar);
        });
    return energy;
  }

  template <class T>
  T compute_triplet_energy_objective(const triplet_tensor_t<count_t>& counts,
                                     const objective_terms<triplet_tensor_t<T>>& terms,
                                     const core::triplet_class_mapping_t& classes) {
    T energy{0};
    detail::for_each_triplet_term(
        counts, classes,
        [&](Eigen::Index c, Eigen::Index xi, Eigen::Index eta, Eigen::Index zeta, count_t nbar) {
          energy += terms.s(c, xi, eta, zeta) * terms.B(c, xi, eta, zeta) * static_cast<T>(nbar);
        });
    return energy;
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_OBJECTIVE_IMPL_HPP
