//
// Created by Dominik Gehringer on 08.01.25.
//

#ifndef SQSGEN_IO_MPI_H
#define SQSGEN_IO_MPI_H

#ifdef WITH_MPI
#include <mpl/mpl.hpp>
#endif

namespace sqsgen::io {
#ifdef WITH_MPI
  static constexpr bool have_mpi = true;
#else
  static constexpr bool have_mpi = false;
#endif

  namespace mpi {
    static constexpr int TAG_BETTER_OBJECTIVE = 1;
  }

  template <class T> class MPICommunicator {
#ifdef WITH_MPI
    mpl::communicator _comm;
#endif
    int _head_rank;
    thread_config_t _config;

  public:
    MPICommunicator(thread_config_t const& config) : _head_rank(0), _config(config) {
      if constexpr (have_mpi) {
        _comm = mpl::environment::comm_world();
      }
    }

    auto rank() const {
      if constexpr (have_mpi) {
        return _comm.rank();
      }
      return 0;
    }

    void synchronize_objective(std::atomic<T>& best_objective) const {
      if constexpr (have_mpi) {
        std::optional<mpl::status_t> status
            = _comm.iprobe(mpl::any_source, mpl::tag_t(mpi::TAG_BETTER_OBJECTIVE));
        while (status.has_value()) {
          T other;
          auto req
              = _comm.irecv(other, status.value().source(), mpl::tag_t(mpi::TAG_BETTER_OBJECTIVE));
          req.wait();
          std::cout << std::format("Rank {} reveived: {} ({})\n", rank(), other, best_objective.load());
          if (other < best_objective.load()) best_objective.exchange(other);
          status = _comm.iprobe(mpl::any_source, mpl::tag_t(mpi::TAG_BETTER_OBJECTIVE));
        }
      }
    }

    void broadcast_objective(T&& objective) const {
      if constexpr (have_mpi) {
        {
          mpl::irequest_pool pool;
          for (auto i = 0; i < num_ranks(); ++i) {
            if (i == this->rank()) continue;
            pool.push(_comm.isend(objective, i, mpl::tag_t(mpi::TAG_BETTER_OBJECTIVE)));
          }
          pool.waitall();
        }
      }
    }

    [[nodiscard]] usize_t num_threads() const {
      if (std::holds_alternative<usize_t>(_config)) {
        auto num_threads = std::get<usize_t>(_config);
        return num_threads > 0 ? num_threads : std::thread::hardware_concurrency();
      } else {
        auto threads_per_rank = std::get<std::vector<usize_t>>(_config);
        if (threads_per_rank.size() != num_ranks())
          throw std::invalid_argument(std::format(
              "The communicator has {} ranks, but the thread configuration contains {} entries",
              num_ranks(), threads_per_rank.size()));
        return threads_per_rank[rank()] > 0 ? threads_per_rank[rank()]
                                            : std::thread::hardware_concurrency();
      }
    }

    bool is_head() { return rank() == _head_rank; }
    auto num_ranks() const {
      if constexpr (have_mpi) {
        return _comm.size();
      }
      return 1;
    }
  };
}  // namespace sqsgen::io

#endif  // SQSGEN_IO_MPI_H
