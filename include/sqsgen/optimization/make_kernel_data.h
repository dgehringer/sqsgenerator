//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_OPTIMIZATION_MAKE_KERNEL_DATA_H
#define SQSGEN_OPTIMIZATION_MAKE_KERNEL_DATA_H

#include "sqsgen/configuration.h"
#include "sqsgen/optimization/kernel_data.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  namespace configuration = sqsgen::configuration;

  // configuration -> kernel data; the only place that knows the sublattice mode and the objective
  // type. Defined in its own file, since it needs the structure and shell code.
  template <class T, sublattice_mode Mode>
  kernel_data<T> make_kernel_data(const configuration::configuration<T, Mode>& config);

}  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/make_kernel_data_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_MAKE_KERNEL_DATA_H
