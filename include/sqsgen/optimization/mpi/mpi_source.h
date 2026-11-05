#ifndef SQSGEN_OPTIMIZATION_MPI_MPI_SOURCE_H
#define SQSGEN_OPTIMIZATION_MPI_MPI_SOURCE_H

#include <mpi.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <variant>

#include "sqsgen/optimization/scheduler.h"
#include "sqsgen/optimization/work_source.h"

// MPI transport for the scheduler. Requires MPI_THREAD_FUNNELED: only the main thread of a
// rank calls MPI.
//
//   head rank:   compute threads   -> core::local_source -> shared_scheduler
//                main thread       -> serve_head: answers requests of the other ranks
//                                     with the same shared_scheduler
//   worker rank: compute threads   -> mpi_source (hand the request to the main thread, wait)
//                main thread       -> serve_worker: forwards requests to the head
//
// Protocol (one tag, one request in flight per worker rank):
//   worker -> head:  request_msg{report}
//   head -> worker:  package_msg{package, global best}  or  done_msg (once per remote thread)
// A rank's serve loop ends when every one of its threads got done_msg.
// The final result gather (MPI_Gatherv) is a separate step after execute().

namespace sqsgen::optimization::mpi {

  namespace optimization = sqsgen::optimization;
  struct request_msg {
    optimization::report report;
  };

  struct package_msg {
    optimization::package package;
    double best;  // global best objective, keeps the gates of all ranks in sync
  };

  struct done_msg {};

  using to_worker_msg = std::variant<package_msg, done_msg>;

  inline constexpr int tag_work = 1;

  // serialize with glaze (BEVE) and send as MPI_BYTE
  template <class Msg> void send(const Msg& message, int destination, MPI_Comm comm);

  // probe the size, receive, deserialize; *from = rank of the sender
  template <class Msg> Msg receive(int source, MPI_Comm comm, int* from = nullptr);

  // work source of the compute threads on a worker rank
  struct mpi_source {
    // one request of one compute thread, lives on that thread's stack until answered
    struct pending {
      optimization::report report;
      std::optional<optimization::package> reply{};
      bool ready{false};
    };

    std::mutex mutex;
    std::condition_variable cv;
    std::deque<pending*> outbox;

    // enqueue the request for the main thread and wait for the answer
    std::optional<optimization::package> next(const optimization::report& r);
  };

  // worker rank, main thread: forward requests until all num_threads threads got done_msg
  template <class T> void serve_worker(mpi_source& source, std::atomic<T>& gate,
                                       std::size_t num_threads, int head, MPI_Comm comm);

  // head rank, main thread: answer requests until all remote threads got done_msg
  // num_remote_consumers = sum of the compute threads of all other ranks
  template <class T> void serve_head(optimization::shared_scheduler& shared, std::atomic<T>& gate,
                                     std::size_t num_remote_consumers, MPI_Comm comm);

}  // namespace sqsgen::optimization::mpi

#include "sqsgen/impl/mpi/mpi_source_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_MPI_MPI_SOURCE_H
