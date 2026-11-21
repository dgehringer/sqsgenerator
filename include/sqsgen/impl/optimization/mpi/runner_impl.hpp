//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_MPI_RUNNER_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_MPI_RUNNER_IMPL_HPP

#include <mpi.h>

#include <cstddef>
#include <stop_token>
#include <utility>

#include "sqsgen/optimization/mpi/runner.h"

namespace sqsgen::optimization::mpi {

  inline std::size_t ranks_on_node(MPI_Comm comm) {
    int rank = 0;
    MPI_Comm_rank(comm, &rank);

    MPI_Comm node;
    MPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &node);
    int count = 1;
    MPI_Comm_size(node, &count);
    MPI_Comm_free(&node);
    return static_cast<std::size_t>(count);
  }

  inline run_context make_mpi_run_context(const configuration::thread_config_t& threads,
                                          std::stop_token stop, progress_callback on_progress,
                                          MPI_Comm comm) {
    int rank = 0;
    int size = 1;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    const std::size_t own = resolve_num_threads(threads, ranks_on_node(comm));

    // sum of the compute threads of all ranks, reduced onto the head (root 0)
    unsigned long long local = own;
    unsigned long long total = 0;
    MPI_Reduce(&local, &total, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, /*root=*/0, comm);

    const mpi_role role = size == 1   ? mpi_role::single
                          : rank == 0 ? mpi_role::head
                                      : mpi_role::worker;
    return run_context{
        .role = role,
        .num_threads = own,
        .num_remote_consumers = role == mpi_role::head ? static_cast<std::size_t>(total) - own : 0,
        .stop = std::move(stop),
        // only the head (and the single-process case) publishes progress; workers report through
        // the scheduler
        .on_progress = role == mpi_role::worker ? progress_callback{} : std::move(on_progress),
    };
  }

}  // namespace sqsgen::optimization::mpi

#endif  // SQSGEN_IMPL_OPTIMIZATION_MPI_RUNNER_IMPL_HPP
