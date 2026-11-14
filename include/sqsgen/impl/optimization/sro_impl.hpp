//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_SRO_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_SRO_IMPL_HPP

#include <cmath>

#include "sqsgen/optimization/sro.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  inline void count_pairs(pair_tensor_t<count_t>& counts, const core::pair_list_t& pairs,
                          const configuration_t& species) {
    counts.setConstant(0);
    for (const core::pair& p : pairs) ++counts(p.shell, species[p.i], species[p.j]);
  }

  // Objective, canonical-cell summation; see compute_pair_objective in sro.h for the formula.
  template <class T> T compute_pair_objective(const pair_tensor_t<count_t>& bonds,
                                              const objective_terms<pair_tensor_t<T>>& terms) {
    const Eigen::Index num_shells = bonds.dimension(0);
    const Eigen::Index num_species = bonds.dimension(1);

    T objective{0};
    for (Eigen::Index s = 0; s < num_shells; ++s)
      for (Eigen::Index xi = 0; xi < num_species; ++xi) {
        // diagonal: same species
        const T n_diag = static_cast<T>(bonds(s, xi, xi));
        objective
            += terms.s(s, xi, xi) * std::abs(terms.A(s, xi, xi) + n_diag * terms.B(s, xi, xi));

        // off-diagonal: sum both cells, since each bond landed in one of them
        for (Eigen::Index eta = xi + 1; eta < num_species; ++eta) {
          const T n = static_cast<T>(bonds(s, xi, eta) + bonds(s, eta, xi));
          objective
              += terms.s(s, xi, eta) * std::abs(terms.A(s, xi, eta) + n * terms.B(s, xi, eta));
        }
      }
    return objective;
  }

  /**
   * sites[0] is opposite the shortest edge, sites[2] opposite the longest;
   * see count_triplets in sro.h for the contract.
   */
  inline void count_triplets(triplet_tensor_t<count_t>& counts,
                             const core::triplet_list_t& triplets, const configuration_t& species) {
    counts.setConstant(0);
    for (const core::triplet& t : triplets)
      ++counts(t.c, species[t.sites[0]], species[t.sites[1]], species[t.sites[2]]);
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_SRO_IMPL_HPP
