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
