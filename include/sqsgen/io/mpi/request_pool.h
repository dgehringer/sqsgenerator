//
// Created by Dominik Gehringer on 23.02.25.
//

#ifndef SQSGEN_IO_MPI_REQUEST_POOL_H
#define SQSGEN_IO_MPI_REQUEST_POOL_H
#include <mpl/mpl.hpp>

#include "sqsgen/io/mpi/requests.h"

namespace sqsgen::io::mpi {

  namespace detail {}  // namespace detail

  static constexpr int RANK_HEAD = 0;

  template <class Message> class inbound_request_pool : mpl::irequest_pool {
    Message buffer_;
    mpl::communicator comm_;
    std::vector<request<Message, detail::inbound_request>> request_data_;
    vset<std::size_t> completed_requests_;

  public:
    explicit inbound_request_pool(mpl::communicator const& comm, Message&& buffer)
        : comm_(comm), buffer_(std::forward<Message>(buffer)) {}

    void probe(int source = mpl::any_source) {
      mpl::tag_t tag(request<Message, detail::inbound_request>::tag);
      std::optional<mpl::status_t> status = comm_.iprobe(source, tag);
      while (status.has_value()) {
        status = comm_.iprobe(source, tag);
        auto req = request<Message, detail::inbound_request>{comm_, std::forward<Message>(buffer_)};
        request_data_.emplace_back(req);
        this->push(req.recv(status));
      }
    }

    template <class Fn> void handle_completed(Fn&& fn) {
      auto [test_result, indices] = this->testsome();
      if (test_result == mpl::test_result::completed)
        for (auto index : indices) {
          if (!completed_requests_.contains(index)) {
            fn(std::forward<Message>(request_data_.at(index).result()));
            completed_requests_.insert(index);
          }
        }
    }

    template <class Fn> void recv_all(Fn&& fn, int source = mpl::any_source) {
      probe(source);
      this->waitall();
      for (auto& req : request_data_) fn(std::forward<Message>(request_data_.at(index).result()));
    }
  };

  template <class Message> class outbound_request_pool : mpl::irequest_pool {
    mpl::communicator comm_;
    std::vector<request<Message, detail::outbound_request>> request_data_;

  public:
    explicit outbound_request_pool(mpl::communicator const& comm = mpl::environment::comm_world())
        : comm_(comm) {}

    void send(Message&& msg, int to) {
      auto req = request<Message, detail::outbound_request>{comm_, std::forward<Message>(msg)};
      this->push(req.send(to));
      request_data_.push_back(req);
    }
  };
}  // namespace sqsgen::io::mpi

#endif  // SQSGEN_IO_MPI_REQUEST_POOL_H
