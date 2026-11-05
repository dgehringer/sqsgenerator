#ifndef SQSGEN_IMPL_OPTIMIZATION_MPI_SOURCE_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_MPI_SOURCE_IMPL_HPP

#include <cassert>
#include <glaze/beve.hpp>
#include <string>

#include "sqsgen/optimization/mpi/mpi_source.h"

namespace sqsgen::optimization::mpi {

  namespace detail {
    template <class... Fs> struct overloaded : Fs... {
      using Fs::operator()...;
    };
  }  // namespace detail

  template <class Msg> void send(const Msg& message, int destination, MPI_Comm comm) {
    std::string buffer;
    [[maybe_unused]] const auto ec = glz::write_beve(message, buffer);
    assert(!ec);  // our own message types: serialization cannot fail
    MPI_Send(buffer.data(), static_cast<int>(buffer.size()), MPI_BYTE, destination, tag_work, comm);
  }

  template <class Msg> Msg receive(int source, MPI_Comm comm, int* from) {
    MPI_Status status;
    MPI_Probe(source, tag_work, comm, &status);
    int size = 0;
    MPI_Get_count(&status, MPI_BYTE, &size);

    std::string buffer(static_cast<std::size_t>(size), '\0');
    // receive from the probed rank, not from `source` (which may be MPI_ANY_SOURCE)
    MPI_Recv(buffer.data(), size, MPI_BYTE, status.MPI_SOURCE, tag_work, comm, MPI_STATUS_IGNORE);

    Msg message{};
    [[maybe_unused]] const auto ec = glz::read_beve(message, buffer);
    assert(!ec);
    if (from != nullptr) *from = status.MPI_SOURCE;
    return message;
  }

  inline std::optional<optimization::package> mpi_source::next(const optimization::report& r) {
    pending request{.report = r};
    std::unique_lock lock(mutex);
    outbox.push_back(&request);
    cv.notify_all();
    cv.wait(lock, [&] { return request.ready; });
    return request.reply;
  }

  template <class T> void serve_worker(mpi_source& source, std::atomic<T>& gate,
                                       std::size_t num_threads, int head, MPI_Comm comm) {
    std::size_t finished = 0;
    while (finished < num_threads) {
      mpi_source::pending* request = nullptr;
      {
        std::unique_lock lock(source.mutex);
        source.cv.wait(lock, [&] { return !source.outbox.empty(); });
        request = source.outbox.front();
        source.outbox.pop_front();
      }

      // MPI outside the lock: compute threads can keep enqueueing meanwhile
      send(request_msg{request->report}, head, comm);
      const auto reply = receive<to_worker_msg>(head, comm);

      std::optional<optimization::package> package;
      std::visit(detail::overloaded{
                     [&](const package_msg& m) {
                       optimization::atomic_min(gate, static_cast<T>(m.best));
                       package = m.package;
                     },
                     [&](const done_msg&) { ++finished; },
                 },
                 reply);

      {
        std::scoped_lock lock(source.mutex);
        request->reply = package;
        request->ready = true;
      }
      source.cv.notify_all();
    }
  }

  template <class T> void serve_head(optimization::shared_scheduler& shared, std::atomic<T>& gate,
                                     std::size_t num_remote_consumers, MPI_Comm comm) {
    std::size_t finished = 0;
    while (finished < num_remote_consumers) {
      int from = 0;
      const auto request = receive<request_msg>(MPI_ANY_SOURCE, comm, &from);

      // remote best -> head gate; the head gate (incl. head threads) -> reply
      optimization::atomic_min(gate, static_cast<T>(request.report.best));

      if (const auto p = optimization::next_package(shared, request.report)) {
        const double best = static_cast<double>(gate.load(std::memory_order_relaxed));
        send(to_worker_msg{package_msg{.package = *p, .best = best}}, from, comm);
      } else {
        send(to_worker_msg{done_msg{}}, from, comm);
        ++finished;
      }
    }
  }

}  // namespace sqsgen::optimization::mpi

#endif  // SQSGEN_IMPL_OPTIMIZATION_MPI_SOURCE_IMPL_HPP
