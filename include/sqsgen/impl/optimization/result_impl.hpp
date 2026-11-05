//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_RESULT_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_RESULT_IMPL_HPP

#include <ankerl/unordered_dense.h>

#include <cstddef>
#include <numeric>

#include "sqsgen/optimization/result.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  // Stores only the canonical half xi <= eta (input is symmetric in the species);
  // see to_pair_sro in result.h for the contract.
  template <class T> pair_sro_t<T> to_pair_sro(const pair_tensor_t<T>& sro) {
    const auto num_shells = static_cast<std::size_t>(sro.dimension(0));
    const auto num_species = static_cast<std::size_t>(sro.dimension(1));

    pair_sro_t<T> out;
    for (std::size_t s = 0; s < num_shells; ++s)
      for (std::size_t xi = 0; xi < num_species; ++xi)
        for (std::size_t eta = xi; eta < num_species; ++eta)  // canonical: xi <= eta
          out.emplace(pair_key{static_cast<std::uint32_t>(s),
                               {static_cast<specie_t>(xi), static_cast<specie_t>(eta)}},
                      sro(s, xi, eta));
    return out;
  }

  // Split mode: one tensor per sublattice -> one map per sublattice.
  template <class T>
  std::vector<pair_sro_t<T>> to_pair_sro(const std::vector<pair_tensor_t<T>>& sro) {
    std::vector<pair_sro_t<T>> out;
    out.reserve(sro.size());
    for (const auto& tensor : sro) out.push_back(to_pair_sro(tensor));
    return out;
  }

  template <class T, sublattice_mode Mode> T sqs_result<T, Mode>::total_objective() const {
    if constexpr (Mode == sublattice_mode::interact) {
      return objective;
    } else if constexpr (Mode == sublattice_mode::split) {
      return std::accumulate(objective.begin(), objective.end(), T{0.0});
    } else {
      static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split,
                    "unhandled sublattice_mode");
    }
  }

  // Skips species orderings that are non-canonical within equal shells;
  // see to_triplet_sro in result.h for the contract.
  template <class T>
  triplet_sro_t<T> to_triplet_sro(const triplet_tensor_t<T>& beta,
                                  const std::vector<std::array<std::uint32_t, 3>>& classes) {
    const auto num_classes = static_cast<std::size_t>(beta.dimension(0));
    const auto num_species = static_cast<std::size_t>(beta.dimension(1));
    assert(classes.size() == num_classes && "class table must match the tensor");

    triplet_sro_t<T> out;
    for (std::size_t c = 0; c < num_classes; ++c) {
      const auto& s = classes[c];
      for (std::size_t x1 = 0; x1 < num_species; ++x1)
        for (std::size_t x2 = 0; x2 < num_species; ++x2)
          for (std::size_t x3 = 0; x3 < num_species; ++x3) {
            // skip non-canonical orderings of interchangeable atoms
            if (s[0] == s[1] && x1 > x2) continue;
            if (s[1] == s[2] && x2 > x3) continue;
            out.emplace(triplet_key{s,
                                    {static_cast<specie_t>(x1), static_cast<specie_t>(x2),
                                     static_cast<specie_t>(x3)}},
                        beta(c, x1, x2, x3));
          }
    }
    return out;
  }

  template <class T> std::vector<triplet_sro_t<T>> to_triplet_sro(
      const std::vector<triplet_tensor_t<T>>& beta,
      const std::vector<std::vector<std::array<std::uint32_t, 3>>>& classes) {
    assert(beta.size() == classes.size());
    std::vector<triplet_sro_t<T>> out;
    out.reserve(beta.size());
    for (std::size_t i = 0; i < beta.size(); ++i)
      out.push_back(to_triplet_sro(beta[i], classes[i]));
    return out;
  }

  struct sqs_result_hash {
    template <class T, sublattice_mode Mode>
    std::size_t operator()(const sqs_result<T, Mode>& r) const noexcept {
      if constexpr (Mode == sublattice_mode::interact) {
        const auto& c = r.configuration;
        return ankerl::unordered_dense::detail::wyhash::hash(c.data(), c.size());
      } else {
        std::size_t seed = 0;
        for (const auto& sublattice : r.configuration)
          seed = hash_combine(seed, ankerl::unordered_dense::detail::wyhash::hash(
                                        sublattice.data(), sublattice.size()));
        return seed;
      }
    }

  private:
    // 64-bit hash combine (boost-style, golden-ratio constant)
    static std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
      return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
    }
  };

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_RESULT_IMPL_HPP
