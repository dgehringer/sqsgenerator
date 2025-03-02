//
// Created by Dominik Gehringer on 19.02.25.
//

#ifndef SQSGEN_IO_MPI_REQUESTS_H
#define SQSGEN_IO_MPI_REQUESTS_H

#include "sqsgen/core/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::io::mpi {
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  namespace detail {

    struct inbound_request {};
    struct outbound_request {};

    template <ranges::range R> auto sizes(R&& r) {
      return r | views::transform([](auto&& v) { return v.size(); });
    }

    template <std::size_t Dim, class R> auto dimension(R&& r) {
      auto d = r.dimensions();
      return d[Dim];
    }

    template <int Tag, class Fn>
    void receive_messages(mpl::communicator& comm, Fn&& fn, int source) {
      std::optional<mpl::status_t> status = comm.iprobe(source, mpl::tag_t{Tag});
      while (status.has_value()) {
        fn(std::forward<mpl::status_t>(status.value()));
        status = comm.iprobe(source, mpl::tag_t{Tag});
      }
    }
  }  // namespace detail

  struct rank_state {
    bool running;
  };

  static constexpr int TAG_STATE = 0;
  static constexpr int TAG_OBJECTIVE = 1;
  static constexpr int TAG_RESULT = 2;
  static constexpr int TAG_STATISTICS = 3;

  template <class, class> class request;

  template <class T, class RequestType>
  class request<sqs_result<T, SUBLATTICE_MODE_SPLIT>, RequestType> {
    using value_t = sqs_result<T, SUBLATTICE_MODE_SPLIT>;

  public:
    static constexpr auto tag = TAG_RESULT;

    void send(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::outbound_request>);
      result_comm(comm, std::forward<value_t>(result), to);
    }
    auto recv(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return result_comm(comm, std::forward<value_t>(result), to);
    }

  private:
    std::vector<std::pair<value_t, int>> result_comm(mpl::communicator& comm, value_t&& result,
                                                     int to) {
      using namespace core::helpers;
      std::vector<T> sro_buff;
      configuration_t species_buff;
      std::vector<T> objective_buff;
      T total_objective;
      std::size_t num_sublattices{result.sublattices.size()};
      std::vector<long> num_shells, num_atoms, num_species;
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>) {
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
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>) {
        auto req = comm.isend(mpl::absolute, l, to, mpl::tag_t(tag));
        req.wait();
        return {};
      } else {
        std::vector<std::pair<value_t, int>> recieved{};
        detail::receive_messages<tag>(
            comm,
            [&, l](auto&& status) {
              auto req = comm.irecv(mpl::absolute, l, status.source(), mpl::tag_t(tag));
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
                  std::make_pair(sqs_result<T, SUBLATTICE_MODE_SPLIT>{total_objective, sublattices},
                                 status.source()));
            },
            to);
        return recieved;
      }
    }
  };

  template <class T, class RequestType>
  class request<sqs_result<T, SUBLATTICE_MODE_INTERACT>, RequestType> {
    using value_t = sqs_result<T, SUBLATTICE_MODE_INTERACT>;

  public:
    static constexpr auto tag = TAG_RESULT;

    void send(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::outbound_request>);
      result_comm(comm, std::forward<value_t>(result), to);
    }
    auto recv(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return result_comm(comm, std::forward<value_t>(result), to);
    }

  private:
    std::vector<std::pair<value_t, int>> result_comm(mpl::communicator& comm, value_t&& result,
                                                     int to) {
      std::vector<T> sro_buff;
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>)
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
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>) {
        auto req = comm.isend(mpl::absolute, l, to, mpl::tag_t(tag));
        req.wait();
        return {};
      } else {
        std::vector<std::pair<value_t, int>> results{};
        detail::receive_messages<tag>(
            comm,
            [&, l](auto&& status) {
              auto req = comm.irecv(mpl::absolute, l, status.source(), mpl::tag_t(tag));
              req.wait();
              result.sro = Eigen::TensorMap<cube_t<T>>(sro_buff.data(), num_shells, num_species,
                                                       num_species);
              results.push_back(std::make_pair(result, status.source()));
            },
            to);
        return results;
      }
    }
  };

  template <class T, class RequestType> class request<objective<T>, RequestType> {
    using value_t = objective<T>;

  public:
    static constexpr auto tag = TAG_OBJECTIVE;

    void send(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::outbound_request>);
      result_comm(comm, std::forward<value_t>(result), to);
    }
    auto recv(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return result_comm(comm, std::forward<value_t>(result), to);
    }

  private:
    std::vector<std::pair<value_t, int>> result_comm(mpl::communicator& comm, value_t&& result,
                                                     int to) {
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>) {
        auto req = comm.isend(result.value, to, mpl::tag_t(tag));
        req.wait();
        return {};
      } else {
        std::vector<std::pair<value_t, int>> results{};
        detail::receive_messages<tag>(
            comm,
            [&](auto&& status) {
              auto req = comm.irecv(result.value, status.source(), mpl::tag_t(tag));
              req.wait();
              results.push_back(std::make_pair(value_t{result.value}, status.source()));
            },
            to);
        return results;
      }
    }
  };

  template <class RequestType> class request<rank_state, RequestType> {
    using value_t = rank_state;

  public:
    static constexpr auto tag = TAG_STATE;

    void send(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::outbound_request>);
      result_comm(comm, std::forward<value_t>(result), to);
    }
    auto recv(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return result_comm(comm, std::forward<value_t>(result), to);
    }

  private:
    std::vector<std::pair<value_t, int>> result_comm(mpl::communicator& comm, value_t&& result,
                                                     int to) {
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>) {
        auto req = comm.isend(result.running, to, mpl::tag_t(tag));
        req.wait();
        return {};
      } else {
        std::vector<std::pair<value_t, int>> results{};
        detail::receive_messages<tag>(
            comm,
            [&](auto&& status) {
              auto req = comm.irecv(result.running, status.source(), mpl::tag_t(tag));
              req.wait();
              results.push_back(std::make_pair(value_t{result.running}, status.source()));
            },
            to);
        return results;
      }
    }
  };

  template <class T, class RequestType> class request<sqs_statistics_data<T>, RequestType> {
    using value_t = sqs_statistics_data<T>;

  public:
    static constexpr auto tag = TAG_STATISTICS;

    void send(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::outbound_request>);
      result_comm(comm, std::forward<value_t>(result), to);
    }
    auto recv(mpl::communicator& comm, value_t&& result, int to) {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return result_comm(comm, std::forward<value_t>(result), to);
    }

  private:
    std::vector<std::pair<value_t, int>> result_comm(mpl::communicator& comm, value_t&& data,
                                                         int to) {
      using namespace core::helpers;
      constexpr auto order = std::array{TIMING_TOTAL, TIMING_COMM, TIMING_CHUNK_SETUP, TIMING_LOOP};
      std::vector<nanoseconds_t> timings(order.size());
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>)
        timings = as<std::vector>{}(
            order | views::transform([&](auto&& t) { return data.timings.at(t); }));

      mpl::vector_layout<nanoseconds_t> timings_layout(timings.size());
      mpl::heterogeneous_layout l(data.finished, data.working, data.best_rank, data.best_objective,
                                  mpl::make_absolute(timings.data(), timings_layout));
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>) {
        auto req = comm.isend(mpl::absolute, l, to, mpl::tag_t(tag));
        req.wait();
        return {};
      } else {
        std::vector<std::pair<value_t, int>> results;
        detail::receive_messages<tag>(
            comm,
            [&, l](auto&& status) {
              auto req = comm.irecv(mpl::absolute, l, status.source(), mpl::tag_t(tag));
              req.wait();
              results.push_back(std::make_pair(
                  sqs_statistics_data<T>{
                      data.finished, data.working, data.best_rank, data.best_objective,
                      as<std::map>{}(range(std::size(order)) | views::transform([&](auto&& i) {
                                       return std::make_pair(order[i], timings[i]);
                                     }))},
                  status.source()));
            },
            to);
        return results;
      }
    }
  };

}  // namespace sqsgen::io::mpi

#endif  // SQSGEN_IO_MPI_REQUESTS_H
