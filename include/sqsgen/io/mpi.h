//
// Created by Dominik Gehringer on 28.02.25.
//

#ifndef SQSGEN_IO_MPI_H
#define SQSGEN_IO_MPI_H

#include <sqsgen/io/mpi/config.h>
#include <sqsgen/io/mpi/requests.h>

namespace sqsgen::io::mpi {
#ifdef WITH_MPI
  template <class Message, class Fn>
  void recv_all(mpl::communicator& comm, Message&& buffer, Fn&& fn, int source = mpl::any_source) {
    for (auto&& [value, rank] : request<Message, detail::inbound_request>{}.recv(
             comm, std::forward<Message>(buffer), source))
      fn(std::forward<decltype(value)>(value), rank);
  }

  template <class Message> void send(mpl::communicator& comm, Message&& message, int to) {
    return request<Message, detail::outbound_request>{}.send(comm, std::forward<Message>(message),
                                                             to);
  }
#endif
}  // namespace sqsgen::io::mpi

#endif  // SQSGEN_IO_MPI_H
