//
// Created by Dominik Gehringer on 08.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_COMM_H
#define SQSGEN_OPTIMIZATION_COMM_H

#ifdef WITH_MPI
#  include <mpl/mpl.hpp>
#endif

namespace sqsgen::optimization::comm {
#ifdef WITH_MPI
  static constexpr bool have_mpi = true;
#else
  static constexpr bool have_mpi = false;
#endif

  static constexpr int TAG_BETTER_OBJECTIVE = 1;
  static constexpr int TAG_RESULT = 2;
  static constexpr int TAG_STATISTICS = 3;

  namespace detail {
    namespace ranges = std::ranges;
    namespace views = ranges::views;
    template <ranges::range R> auto sizes(R&& r) {
      return r | views::transform([](auto&& v) { return v.size(); });
    }

    template <std::size_t Dim, class R> auto dimension(R&& r) {
      auto d = r.dimensions();
      return d[Dim];
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

    void send_statistics(sqs_statistics_data<T>&& data) {
      if constexpr (have_mpi) {
        statistics_comm<SEND>(std::forward<sqs_statistics_data>(data));
      }
    }

    auto pull_statistics(sqs_statistics_data<T>&& data) {
      if constexpr (have_mpi) {
        return statistics_comm<RECV>(std::forward<sqs_statistics_data>(data));
      } else
        return {};
    }

    template <SublatticeMode Mode> void send_result(sqs_result<T, Mode>&& result) {
      if constexpr (have_mpi) {
        result_comm<SEND>(_comm, _head_rank, std::move(result));
      }
    }

    template <SublatticeMode Mode> auto pull_results(sqs_result<T, Mode>& buffer) {
      if constexpr (have_mpi) {
        return result_comm<RECV>(_comm, _head_rank, std::move(buffer));
      } else
        return {};
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

    void barrier() {
      if constexpr (have_mpi) {
        _comm.barrier();
      }
    }

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
    template <bool Mode> auto statistics_comm(sqs_statistics_data<T>&& data) {
      std::vector<std::pair<std::string, nanoseconds_t>> timings;
      if constexpr (Mode == SEND) timings = std::vector{data.timings};

      mpl::vector_layout<std::pair<std::string, nanoseconds_t>> timings_layout(timings.size());
      mpl::heterogeneous_layout l(data.finished, data.working, data.best_rank, data.best_objective,
                                  mpl::make_absolute(timings.data(), timings_layout));
      if constexpr (Mode == SEND) {
        auto req = _comm.isend(mpl::absolute, l, _head_rank, mpl::tag_t(TAG_STATISTICS));
        req.wait();
        return std::vector<sqs_statistics_data<T>>{};
      } else {
        std::vector<sqs_statistics_data<T>> results;
        receive_messages(
            [&, l](auto&& status) {
              auto req = _comm.irecv(mpl::absolute, l, status.source(), mpl::tag_t(TAG_STATISTICS));
              req.wait();
              results.push_back(
                  {data.finished, data.working, data.best_rank, data.best_objective, timings});
            },
            mpl::tag_t(TAG_STATISTICS), mpl::any_source);
        return results;
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
      auto num_shells{detail::dimension<0>(result.sro)},
          num_species{detail::dimension<1>(result.sro)};
      assert(result.sro.size() == num_shells * num_species * num_species);
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
      T total_objective;
      std::size_t num_sublattices{result.sublattices.size()};
      std::vector<long> num_shells, num_atoms, num_species;
      if constexpr (Mode == SEND) {
        for (auto sl : result.sublattices) {
          sro_buff.insert(sro_buff.end(), sl.sro.data(), sl.sro.data() + sl.sro.size());
          species_buff.insert(species_buff.end(), sl.species.begin(), sl.species.end());
          objective_buff.push_back(sl.objective);
          num_shells.push_back(detail::dimension<0>(sl.sro));
          num_species.push_back(detail::dimension<1>(sl.sro));
          num_atoms = as<std::vector>{}(result.sublattices | views::transform([&](auto&& s) {
                                          return static_cast<long>(s.species.size());
                                        }));
        }
        total_objective = result.objective;
        assert(num_atoms.size() == num_sublattices);
        assert(num_species.size() == num_sublattices);
        assert(num_shells.size() == num_sublattices);
      } else {
        sro_buff.resize(
            sum(result.sublattices | views::transform([&](auto r) { return r.sro.size(); })));
        species_buff.resize(
            sum(result.sublattices | views::transform([&](auto r) { return r.species.size(); })));
        objective_buff.resize(num_sublattices);
        num_atoms.resize(num_sublattices);
        num_species.resize(num_sublattices);
        num_shells.resize(num_sublattices);
      }
      mpl::vector_layout<T> sro_layout(sro_buff.size());
      mpl::vector_layout<T> objective_layout(objective_buff.size());
      mpl::vector_layout<specie_t> species_layout(species_buff.size());
      mpl::vector_layout<long> num_atoms_layout(num_atoms.size());
      mpl::vector_layout<long> num_species_layout(num_species.size());
      mpl::vector_layout<long> num_shells_layout(num_shells.size());
      mpl::heterogeneous_layout l(total_objective, num_sublattices,
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
        sqs_result_vector_t<SUBLATTICE_MODE_SPLIT> recieved{};
        receive_messages(
            [&, l](auto&& status) {
              auto req = comm.irecv(mpl::absolute, l, status.source(), mpl::tag_t(TAG_RESULT));
              req.wait();
              std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>> sublattices;
              long offset_sro{0}, offset_species{0};
              for (auto sigma = 0; sigma < num_sublattices; ++sigma) {
                sublattices.push_back(sqs_result<T, SUBLATTICE_MODE_INTERACT>{
                    objective_buff[sigma],
                    configuration_t(species_buff.begin() + offset_species,
                                    species_buff.begin() + offset_sro + num_species[sigma]),
                    cube_t<T>(Eigen::TensorMap<cube_t<T>>(sro_buff.data() + offset_sro,
                                                          num_shells[sigma], num_species[sigma],
                                                          num_species[sigma]))});
                offset_species += num_species[sigma];
                offset_sro += num_shells[sigma] * num_species[sigma] * num_species[sigma];
              }
              recieved.push_back(
                  sqs_result<T, SUBLATTICE_MODE_SPLIT>{total_objective, sublattices});
            },
            mpl::tag_t(TAG_RESULT), mpl::any_source);
        return recieved;
      }
    }
  };
}  // namespace sqsgen::optimization::comm

#endif  // SQSGEN_OPTIMIZATION_COMM_H
