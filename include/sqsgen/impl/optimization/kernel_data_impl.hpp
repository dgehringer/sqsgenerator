//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_KERNEL_DATA_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_KERNEL_DATA_IMPL_HPP

#include <cstddef>
#include <utility>
#include <vector>

#include "sqsgen/optimization/kernel_data.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  namespace core = sqsgen::core;

  // ======================================================================= scratch

  template <class T> kernel_scratch make_scratch(const kernel_data<T>& data) {
    kernel_scratch scratch;
    scratch.lattices.reserve(data.lattices.size());
    for (const lattice_data<T>& lattice : data.lattices) {
      lattice_scratch ls;
      ls.configuration = lattice.species_packed;    // current = initial configuration
      ls.generators.resize(lattice.ranges.size());  // one generator per shuffled range
      if (lattice.pairs.has_value()) {
        const pair_tensor_t<T>& a = lattice.pairs->terms.A;
        ls.pair_counts = pair_tensor_t<count_t>(a.dimension(0), a.dimension(1), a.dimension(2));
        ls.pair_counts.setZero();
      }
      if (lattice.triplets.has_value()) {
        const triplet_tensor_t<T>& a = lattice.triplets->terms.A;
        ls.triplet_counts = triplet_tensor_t<count_t>(a.dimension(0), a.dimension(1),
                                                      a.dimension(2), a.dimension(3));
        ls.triplet_counts.setZero();
      }
      scratch.lattices.push_back(std::move(ls));
    }
    return scratch;
  }

  // ======================================================================= coefficients

  template <class Tensor, class T>
  objective_terms<Tensor> sro_terms(const Tensor& prefactors, const Tensor& weights,
                                    const Tensor& target, T scale) {
    using scalar = typename Tensor::Scalar;
    // omega = scale * weights; folds the global (1 - lambda)/W (or lambda/W) into the per-cell
    // weight
    const Tensor omega = weights * weights.constant(static_cast<scalar>(scale));

    objective_terms<Tensor> terms;
    terms.A = omega * (target - target.constant(scalar{1}));  // a = omega (alpha_target - 1)
    terms.B = omega * prefactors;                             // b = omega f
    // s = 1 where the term contributes, 0 where omega vanishes (selected shell with no sites)
    terms.s = omega;
    scalar* s = terms.s.data();
    for (Eigen::Index i = 0; i < terms.s.size(); ++i)
      s[i] = s[i] != scalar{0} ? scalar{1} : scalar{0};
    return terms;
  }

  template <class Tensor> objective_terms<Tensor> energy_terms(const Tensor& interactions) {
    using scalar = typename Tensor::Scalar;
    objective_terms<Tensor> terms;
    terms.A = interactions.constant(scalar{0});  // energy has no constant offset
    terms.B = interactions.abs();                // b = |J|
    // s = sign(J), so s |A + N B| = s |N J| = N J (N >= 0); s = 0 drops a vanishing interaction
    terms.s = interactions;
    scalar* s = terms.s.data();
    for (Eigen::Index i = 0; i < terms.s.size(); ++i)
      s[i] = s[i] > scalar{0} ? scalar{1} : (s[i] < scalar{0} ? scalar{-1} : scalar{0});
    return terms;
  }

  // ======================================================================= map -> tensor

  template <class T> pair_tensor_t<T> to_pair_tensor(const pair_sro_t<T>& values,
                                                     const std::vector<shell_t>& shells,
                                                     const configuration_t& species, T missing) {
    const auto num_shells = static_cast<Eigen::Index>(shells.size());
    const auto num_species = static_cast<Eigen::Index>(species.size());
    pair_tensor_t<T> tensor(num_shells, num_species, num_species);
    tensor.setConstant(missing);
    for (Eigen::Index s = 0; s < num_shells; ++s)
      for (Eigen::Index xi = 0; xi < num_species; ++xi)
        for (Eigen::Index eta = 0; eta < num_species; ++eta) {
          const core::pair_key key(shells[s], {species[xi], species[eta]});
          if (const auto it = values.find(key); it != values.end()) tensor(s, xi, eta) = it->second;
        }
    return tensor;
  }

  template <class T>
  triplet_tensor_t<T> to_triplet_tensor(const triplet_sro_t<T>& values,
                                        const core::triplet_class_mapping_t& classes,
                                        const configuration_t& species, T missing) {
    const auto num_classes = static_cast<Eigen::Index>(classes.size());
    const auto num_species = static_cast<Eigen::Index>(species.size());
    triplet_tensor_t<T> tensor(num_classes, num_species, num_species, num_species);
    tensor.setConstant(missing);
    for (Eigen::Index c = 0; c < num_classes; ++c)
      for (Eigen::Index xi = 0; xi < num_species; ++xi)
        for (Eigen::Index eta = 0; eta < num_species; ++eta)
          for (Eigen::Index zeta = 0; zeta < num_species; ++zeta) {
            const core::triplet_key key(classes[c], {species[xi], species[eta], species[zeta]});
            if (const auto it = values.find(key); it != values.end())
              tensor(c, xi, eta, zeta) = it->second;
          }
    return tensor;
  }

  // ======================================================================= tensor -> map

  template <class T> pair_sro_t<T> to_pair_sro(const pair_tensor_t<T>& tensor,
                                               const std::vector<shell_t>& shells,
                                               const std::vector<specie_t>& species) {
    const auto num_shells = static_cast<Eigen::Index>(shells.size());
    const auto num_species = static_cast<Eigen::Index>(species.size());
    pair_sro_t<T> out;
    for (Eigen::Index s = 0; s < num_shells; ++s)
      for (Eigen::Index xi = 0; xi < num_species; ++xi)
        for (Eigen::Index eta = xi; eta < num_species; ++eta)  // canonical cell xi <= eta
          out.emplace(core::pair_key(shells[s], {species[xi], species[eta]}), tensor(s, xi, eta));
    return out;
  }

  template <class T> triplet_sro_t<T> to_triplet_sro(const triplet_tensor_t<T>& tensor,
                                                     const core::triplet_class_mapping_t& classes,
                                                     const std::vector<specie_t>& species) {
    const auto num_classes = static_cast<Eigen::Index>(classes.size());
    const auto num_species = static_cast<Eigen::Index>(species.size());
    triplet_sro_t<T> out;
    for (Eigen::Index c = 0; c < num_classes; ++c)
      for (Eigen::Index xi = 0; xi < num_species; ++xi)
        for (Eigen::Index eta = 0; eta < num_species; ++eta)
          for (Eigen::Index zeta = 0; zeta < num_species; ++zeta)
            // triplet_key canonicalizes; the first representative of each class wins
            out.emplace(core::triplet_key(classes[c], {species[xi], species[eta], species[zeta]}),
                        tensor(c, xi, eta, zeta));
    return out;
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_KERNEL_DATA_IMPL_HPP
