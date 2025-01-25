//
// Created by Dominik Gehringer on 08.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_COMM_H
#define SQSGEN_OPTIMIZATION_COMM_H

#ifdef WITH_MPI
#  include <mpl/mpl.hpp>
#endif
#include "sqsgen/optimization/collection.h"

namespace sqsgen::optimization::comm {
#ifdef WITH_MPI
  static constexpr bool have_mpi = true;
#else
  static constexpr bool have_mpi = false;
#endif

  static constexpr int TAG_BETTER_OBJECTIVE = 1;
  static constexpr int TAG_RESULT = 2;

  namespace detail {
    namespace ranges = std::ranges;
    namespace views = ranges::views;
    template <ranges::range R> auto sizes(R&& r) {
      return r | views::transform([](auto&& v) { return v.size(); });
    }

    template <std::size_t Dim, ranges::range R> auto dimension(R&& r) {
      return r | views::transform([](auto&& v) {
               auto d = v.dimensions();
               return d[Dim];
             });
    }

  }  // namespace detail

  template <class T> class MPICommunicator {
#ifdef WITH_MPI
    mpl::communicator _comm;
#endif
    int _head_rank;
    thread_config_t _config;

  public:
    explicit MPICommunicator(thread_config_t config) : _head_rank(0), _config(std::move(config)) {
      if constexpr (have_mpi) {
        using namespace mpl;
        _comm = environment::comm_world();
        if (environment::threading_mode() != threading_modes::funneled
            && environment::threading_mode() != threading_modes::multiple) {
          _comm.abort(-1);
        }
      }
    }

    auto rank() const {
      if constexpr (have_mpi) {
        return _comm.rank();
      }
      return 0;
    }

    void pull_objective(std::atomic<T>& best_objective) {
      mpl::tag_t tag(TAG_BETTER_OBJECTIVE);
      receive_messages(
          [&best_objective, this, tag](auto&& status) {
            T other;
            auto req = _comm.irecv(other, status.source(), tag);
            req.wait();
            if (other < best_objective.load()) best_objective.exchange(other);
          },
          tag);
    }

    void broadcast_objective(T&& objective) const {
      if constexpr (have_mpi) {
        {
          mpl::irequest_pool pool;
          core::helpers::for_each(
              [this, &pool, objective](auto&& r) {
                if (r != rank())
                  pool.push(_comm.isend(objective, r, mpl::tag_t(TAG_BETTER_OBJECTIVE)));
              },
              num_ranks());
          pool.waitall();
        }
      }
    }

    template <SublatticeMode Mode> void send_result(sqs_result<T, Mode>&& result) {
      if constexpr (have_mpi) {
        result_comm<SEND>(_comm, _head_rank, std::move(result));
      }
    }

    template <SublatticeMode Mode> auto pull_results(sqs_result<T, Mode>& buffer) {
      if constexpr (have_mpi) {
        return result_comm<RECV>(_comm, _head_rank, std::move(buffer));
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

  private:
    static constexpr bool SEND = true;
    static constexpr bool RECV = false;

    template <SublatticeMode Mode> using sqs_result_t = sqs_result<T, Mode>;

    template <SublatticeMode Mode> using sqs_result_vector_t = std::vector<sqs_result_t<Mode>>;

    template <class Fn>
    void receive_messages(Fn&& fn, mpl::tag_t tag, int source = mpl::any_source) {
      if constexpr (have_mpi) {
        std::optional<mpl::status_t> status = _comm.iprobe(source, tag);
        while (status.has_value()) {
          fn(std::forward<mpl::status_t>(status.value()));
          status = _comm.iprobe(source, tag);
        }
      }
    }

    template <bool Mode> sqs_result_vector_t<SUBLATTICE_MODE_INTERACT> result_comm(
        mpl::communicator& comm, int to, sqs_result_t<SUBLATTICE_MODE_INTERACT>&& result) {
      std::vector<T> sro_buff;
      if constexpr (Mode == SEND)
        sro_buff = std::vector(result.sro.data(), result.sro.data() + result.sro.size());
      else
        sro_buff.resize(result.sro.size());
      mpl::vector_layout<T> sro_layout(result.sro.size());
      mpl::vector_layout<specie_t> species_layout(result.species.size());
      std::size_t num_atoms{result.species.size()};
      const typename cube_t<T>::Dimensions& d = result.sro.dimensions();
      auto num_shells{d[0]}, num_species{d[1]};
      assert(result.sro.size() == d[0] * d[1] * d[2]);
      mpl::heterogeneous_layout l(
          result.objective, num_atoms, mpl::make_absolute(result.species.data(), species_layout),
          num_shells, num_species, mpl::make_absolute(sro_buff.data(), sro_layout));
      if constexpr (Mode == SEND) {
        auto req = comm.isend(mpl::absolute, l, to, mpl::tag_t(TAG_RESULT));
        req.wait();
        return {};
      } else {
        sqs_result_vector_t<SUBLATTICE_MODE_INTERACT> results{};
        receive_messages(
            [&, l](auto&& status) {
              auto req = comm.irecv(mpl::absolute, l, status.source(), mpl::tag_t(TAG_RESULT));
              req.wait();
              result.sro = Eigen::TensorMap<cube_t<T>>(sro_buff.data(), num_shells, num_species,
                                                       num_species);
              results.push_back(result);
            },
            mpl::tag_t(TAG_RESULT), mpl::any_source);
        return results;
      }
    }

    template <bool Mode> sqs_result_vector_t<SUBLATTICE_MODE_SPLIT> result_comm(
        mpl::communicator& comm, int to, sqs_result_t<SUBLATTICE_MODE_SPLIT>&& result) {
      using namespace core::helpers;
      std::vector<T> sro_buff;
      configuration_t species_buff;
      std::vector<T> objective_buff;
      assert(result.sro.size() == result.species.size());
      assert(result.sro.size() == result.objective.size());
      std::size_t num_sublattices{result.species.size()};
      std::vector<std::size_t> num_shells, num_atoms, num_species;
      if constexpr (Mode == SEND) {
        for (auto sro : result.sro)
          sro_buff.insert(sro_buff.end(), sro.data(), sro.data() + sro.size());
        for (auto species : result.species)
          species_buff.insert(species_buff.end(), species.begin(), species.end());
        for (auto objective : result.objective) objective_buff.push_back(objective);
        num_shells = as<std::vector>{}(detail::dimension<0>(result.sro));
        num_species = as<std::vector>{}(detail::dimension<1>(result.sro));
        num_atoms = as<std::vector>{}(detail::sizes(result.species));
        assert(num_atoms.size() == num_sublattices);
        assert(num_species.size() == num_sublattices);
        assert(num_shells.size() == num_sublattices);
      } else {
        sro_buff.resize(sum(detail::sizes(result.sro)));
        species_buff.resize(sum(detail::sizes(result.species)));
        objective_buff.resize(num_sublattices);
        num_atoms.resize(num_sublattices);
        num_species.resize(num_sublattices);
        num_shells.resize(num_sublattices);
      }
      mpl::vector_layout<T> sro_layout(sro_buff.size());
      mpl::vector_layout<T> objective_layout(objective_buff.size());
      mpl::vector_layout<specie_t> species_layout(species_buff.size());
      mpl::vector_layout<std::size_t> num_atoms_layout(num_atoms.size());
      mpl::vector_layout<std::size_t> num_species_layout(num_species.size());
      mpl::vector_layout<std::size_t> num_shells_layout(num_shells.size());
      mpl::heterogeneous_layout l(num_sublattices,
                                  mpl::make_absolute(num_atoms.data(), num_atoms_layout),
                                  mpl::make_absolute(objective_buff.data(), objective_layout),
                                  mpl::make_absolute(species_buff.data(), species_layout),
                                  mpl::make_absolute(num_shells.data(), num_shells_layout),
                                  mpl::make_absolute(num_species.data(), num_species_layout),
                                  mpl::make_absolute(sro_buff.data(), sro_layout));
      if constexpr (Mode == SEND) {
        auto req = comm.isend(mpl::absolute, l, to, mpl::tag_t(TAG_RESULT));
        req.wait();
        return {};
      } else {
        sqs_result_vector_t<SUBLATTICE_MODE_INTERACT> results{};
        receive_messages(
            [&, l](auto&& status) {
              auto req = comm.irecv(mpl::absolute, l, status.source(), mpl::tag_t(TAG_RESULT));
              req.wait();
              for (auto sigma = 0; sigma < num_sublattices; ++sigma) {

              }
              result.sro = Eigen::TensorMap<cube_t<T>>(sro_buff.data(), num_shells, num_species,
                                                       num_species);
              results.push_back(result);
            },
            mpl::tag_t(TAG_RESULT), mpl::any_source);
        return results;
      }
    }
  };
}  // namespace sqsgen::optimization::comm

#endif  // SQSGEN_OPTIMIZATION_COMM_H
