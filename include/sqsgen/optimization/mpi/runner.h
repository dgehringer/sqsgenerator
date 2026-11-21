//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_OPTIMIZATION_MPI_RUNNER_H
#define SQSGEN_OPTIMIZATION_MPI_RUNNER_H

#include <mpi.h>

#include <cstddef>
#include <stop_token>

#include "sqsgen/configuration.h"
#include "sqsgen/optimization/runner.h"

/**
 * @file
 * @brief Run context for distributed runs. Requires MPI_THREAD_FUNNELED.
 */

namespace sqsgen::optimization::mpi {

  namespace configuration = sqsgen::configuration;

  /**
   * @brief Number of ranks on the node of the calling rank.
   *
   * Uses MPI_Comm_split_type(MPI_COMM_TYPE_SHARED). Collective over @p comm.
   */
  std::size_t ranks_on_node(MPI_Comm comm);

  /**
   * @brief Run context of the calling rank.
   *
   *   role:                  rank 0 = head, others = worker; a single rank = single
   *   num_threads:           resolve_num_threads(threads, ranks_on_node)
   *   num_remote_consumers:  head only: sum of the threads of all other ranks (MPI_Reduce)
   *   on_progress:           kept on the head only; workers report through the scheduler
   *
   * Collective over @p comm; call after MPI_Init_thread.
   *
   * @param threads      Validated thread configuration (threads per rank).
   * @param stop         Token of this rank's stop source.
   * @param on_progress  Progress callback; only the head calls it.
   * @param comm         Communicator of the run, usually MPI_COMM_WORLD.
   */
  run_context make_mpi_run_context(const configuration::thread_config_t& threads,
                                   std::stop_token stop, progress_callback on_progress,
                                   MPI_Comm comm);

}  // namespace sqsgen::optimization::mpi

#include "sqsgen/impl/optimization/mpi/runner_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_MPI_RUNNER_H
