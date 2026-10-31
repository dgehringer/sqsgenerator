//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_RESULT_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_RESULT_IMPL_HPP

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

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_RESULT_IMPL_HPP
