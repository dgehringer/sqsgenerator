//
// Created by Dominik Gehringer on 19.02.25.
//

#ifndef SQSGEN_IO_MPI_REQUESTS_H
#define SQSGEN_IO_MPI_REQUESTS_H

#include <sqsgen/core/helpers/as.h>

#include <mpl/mpl.hpp>

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
  }  // namespace detail

  template <int, class T, class> class request_base {
  protected:
    mpl::communicator _comm;

  public:
    request_base(mpl::communicator const& comm) : _comm(comm) {}
  };

  static constexpr int TAG_OBJECTIVE = 1;
  static constexpr int TAG_RESULT = 2;
  static constexpr int TAG_STATISTICS = 3;

  template <class, class> class request;

  template <class T, class RequestType>
  class request<sqs_result<T, SUBLATTICE_MODE_SPLIT>, RequestType>
      : public request_base<TAG_RESULT, sqs_result<T, SUBLATTICE_MODE_SPLIT>, RequestType> {
    using value_t = sqs_result<T, SUBLATTICE_MODE_SPLIT>;
    using request_base<TAG_RESULT, value_t, RequestType>::request_base;

    std::vector<T> _sro_buff;
    configuration_t _species_buff;
    std::vector<T> _objective_buff;
    T _total_objective;
    std::size_t _num_sublattices;
    std::vector<long> _num_shells, _num_atoms, _num_species;
    mpl::heterogeneous_layout _layout;

  public:
    static constexpr auto tag = TAG_RESULT;

    request(mpl::communicator const& comm, value_t&& buffer)
        : request_base<tag, value_t, RequestType>(comm) {
      _num_sublattices = buffer.sublattices.size();
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>) {
        using namespace core::helpers;
        for (auto sl : buffer.sublattices) {
          _sro_buff.insert(_sro_buff.end(), sl.sro.data(), sl.sro.data() + sl.sro.size());
          _species_buff.insert(_species_buff.end(), sl.species.begin(), sl.species.end());
          _objective_buff.push_back(sl.objective);
          _num_shells.push_back(detail::dimension<0>(sl.sro));
          _num_species.push_back(detail::dimension<1>(sl.sro));
          _num_atoms = as<std::vector>{}(buffer.sublattices | views::transform([&](auto&& s) {
                                           return static_cast<long>(s.species.size());
                                         }));
        }
        _total_objective = buffer.objective;
        assert(_num_atoms.size() == _num_sublattices);
        assert(_num_species.size() == _num_sublattices);
        assert(_num_shells.size() == _num_sublattices);
      } else if constexpr (std::is_same_v<RequestType, detail::inbound_request>) {
        _sro_buff.resize(
            sum(buffer.sublattices | views::transform([&](auto r) { return r.sro.size(); })));
        _species_buff.resize(
            sum(buffer.sublattices | views::transform([&](auto r) { return r.species.size(); })));
        _objective_buff.resize(_num_sublattices);
        _num_atoms.resize(_num_sublattices);
        _num_species.resize(_num_sublattices);
        _num_shells.resize(_num_sublattices);
      }
      mpl::vector_layout<T> sro_layout(_sro_buff.size());
      mpl::vector_layout<T> objective_layout(_objective_buff.size());
      mpl::vector_layout<specie_t> species_layout(_species_buff.size());
      mpl::vector_layout<long> num_atoms_layout(_num_atoms.size());
      mpl::vector_layout<long> num_species_layout(_num_species.size());
      mpl::vector_layout<long> num_shells_layout(_num_shells.size());
      _layout
          = mpl::heterogeneous_layout(_total_objective, _num_sublattices,
                                      mpl::make_absolute(_num_atoms.data(), num_atoms_layout),
                                      mpl::make_absolute(_objective_buff.data(), objective_layout),
                                      mpl::make_absolute(_species_buff.data(), species_layout),
                                      mpl::make_absolute(_num_shells.data(), num_shells_layout),
                                      mpl::make_absolute(_num_species.data(), num_species_layout),
                                      mpl::make_absolute(_sro_buff.data(), sro_layout));
    }

    mpl::irequest send(int to) {
      static_assert(std::is_same_v<RequestType, detail::outbound_request>);
      return this->_comm.isend(mpl::absolute, _layout, to, mpl::tag_t(tag));
    }
    mpl::irequest recv(mpl::status_t status) {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return this->_comm.irecv(mpl::absolute, _layout, status.source(), mpl::tag_t(tag));
    }

    value_t result() {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>> sublattices;
      long offset_sro{0}, offset_species{0};
      for (auto sigma = 0; sigma < _num_sublattices; ++sigma) {
        sublattices.push_back(sqs_result<T, SUBLATTICE_MODE_INTERACT>{
            _objective_buff[sigma],
            configuration_t(_species_buff.begin() + offset_species,
                            _species_buff.begin() + offset_sro + _num_species[sigma]),
            cube_t<T>(Eigen::TensorMap<cube_t<T>>(_sro_buff.data() + offset_sro, _num_shells[sigma],
                                                  _num_species[sigma], _num_species[sigma]))});
        offset_species += _num_species[sigma];
        offset_sro += _num_shells[sigma] * _num_species[sigma] * _num_species[sigma];
      }
      return sqs_result<T, SUBLATTICE_MODE_SPLIT>{_total_objective, sublattices};
    }
  };

  template <class T, class RequestType>
  class request<sqs_result<T, SUBLATTICE_MODE_INTERACT>, RequestType>
      : public request_base<TAG_RESULT, sqs_result<T, SUBLATTICE_MODE_INTERACT>, RequestType> {
    using value_t = sqs_result<T, SUBLATTICE_MODE_INTERACT>;
    using request_base<TAG_RESULT, value_t, RequestType>::request_base;
    T _objective;
    std::vector<T> _sro_buff;
    configuration_t _species;
    std::size_t _num_atoms, _num_species, _num_shells;
    mpl::heterogeneous_layout _layout;

  public:
    static constexpr auto tag = TAG_RESULT;

    request(mpl::communicator const& comm, value_t&& buffer)
        : request_base<tag, value_t, RequestType>(comm) {
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>) {
        _sro_buff = std::vector(buffer.sro.data(), buffer.sro.data() + buffer.sro.size());
        _species = buffer.species;
      } else if constexpr (std::is_same_v<RequestType, detail::inbound_request>) {
        _sro_buff.resize(buffer.sro.size());
        _species.resize(buffer.species.size());
      }

      mpl::vector_layout<T> sro_layout(buffer.sro.size());
      mpl::vector_layout<specie_t> species_layout(buffer.species.size());
      _num_atoms = buffer.species.size();
      _num_shells = detail::dimension<0>(buffer.sro);
      _num_species = detail::dimension<1>(buffer.sro);
      assert(buffer.sro.size() == _num_shells * _num_species * _num_species);
      _layout = mpl::heterogeneous_layout(
          _objective, _num_atoms, mpl::make_absolute(_species.data(), species_layout), _num_shells,
          _num_species, mpl::make_absolute(_sro_buff.data(), sro_layout));
    }

    mpl::irequest send(int to) {
      static_assert(std::is_same_v<RequestType, detail::outbound_request>);
      return this->_comm.isend(mpl::absolute, _layout, to, mpl::tag_t(tag));
    }
    mpl::irequest recv(mpl::status_t status) {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return this->_comm.irecv(mpl::absolute, _layout, status.source(), mpl::tag_t(tag));
    }

    value_t result() {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return sqs_result<T, SUBLATTICE_MODE_INTERACT>{
          _objective, _species,
          Eigen::TensorMap<cube_t<T>>(_sro_buff.data(), _num_shells, _num_species, _num_species)};
    };
  };

  template <class T, class RequestType> class request<objective<T>, RequestType>
      : public request_base<TAG_OBJECTIVE, objective<T>, RequestType> {
    using value_t = objective<T>;
    using request_base<TAG_OBJECTIVE, value_t, RequestType>::request_base;
    T value_;

  public:
    static constexpr auto tag = TAG_OBJECTIVE;

    request(mpl::communicator const& comm, value_t&& buffer)
        : value_(buffer.value), request_base<tag, value_t, RequestType>(comm) {}

    request(mpl::communicator const& comm, T&& buffer)
        : value_(buffer.value), request_base<tag, value_t, RequestType>(comm) {}

    mpl::irequest send(int to) {
      static_assert(std::is_same_v<RequestType, detail::outbound_request>);
      return this->_comm.isend(value_, to, mpl::tag_t(tag));
    }
    mpl::irequest recv(mpl::status_t status) {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return this->_comm.irecv(value_, status.source(), mpl::tag_t(tag));
    }
  };

  template <class T, class RequestType> class request<sqs_statistics_data<T>, RequestType>
      : public request_base<TAG_STATISTICS, objective<T>, RequestType> {
    using value_t = objective<T>;
    using request_base<TAG_STATISTICS, value_t, RequestType>::request_base;
    value_t buffer_;
    std::vector<nanoseconds_t> timings_;
    mpl::heterogeneous_layout layout_;
    static constexpr auto order
        = std::array{TIMING_TOTAL, TIMING_SYNC, TIMING_CHUNK_SETUP, TIMING_LOOP};

  public:
    static constexpr auto tag = TAG_STATISTICS;

    request(mpl::communicator const& comm, value_t&& buffer)
        : buffer_(buffer.value), request_base<tag, value_t, RequestType>(comm) {
      using namespace core::helpers;
      if constexpr (std::is_same_v<RequestType, detail::outbound_request>)
        timings_ = as<std::vector>{}(
            order | views::transform([&](auto&& t) { return buffer.timings.at(t); }));
      else
        timings_.resize(order.size());
      mpl::vector_layout<nanoseconds_t> timings_layout(timings_.size());
      layout_ = mpl::heterogeneous_layout(buffer_.finished, buffer_.working, buffer_.best_rank,
                                          buffer_.best_objective,
                                          mpl::make_absolute(timings_.data(), timings_layout));
    }

    mpl::irequest send(int to) {
      static_assert(std::is_same_v<RequestType, detail::outbound_request>);
      return this->_comm.isend(mpl::absolute, layout_, to, mpl::tag_t(tag));
    }
    mpl::irequest recv(mpl::status_t status) {
      static_assert(std::is_same_v<RequestType, detail::inbound_request>);
      return this->_comm.irecv(mpl::absolute, layout_, status.source(), mpl::tag_t(tag));
    }
  };

  template <class Message>
  mpl::irequest send(Message&& message, int to,
                     mpl::communicator const& comm = mpl::environment::comm_world()) {
    return request<Message, detail::outbound_request>{comm, std::forward<Message>(message)}.send(
        to);
  }
}  // namespace sqsgen::io::mpi

#endif  // SQSGEN_IO_MPI_REQUESTS_H
