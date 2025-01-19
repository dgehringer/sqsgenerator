//
// Created by Dominik Gehringer on 09.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_COLLECTION_H
#define SQSGEN_OPTIMIZATION_COLLECTION_H

#include "sqsgen/core/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  template <class, SublatticeMode> struct sqs_result {};

  template <class T> struct sqs_result<T, SUBLATTICE_MODE_INTERACT> {
    std::optional<rank_t> rank;
    T objective;
    configuration_t species;
    cube_t<T> sro;

    static sqs_result empty(shell_weights_t<T> const &weights,
                            core::structure<T> const &structure) {
      return {std::nullopt, std::numeric_limits<T>::infinity(), configuration_t(structure.species),
              cube_t<T>(weights.size(), structure.num_species, structure.num_species)};
    }
  };

  template <class T> struct sqs_result<T, SUBLATTICE_MODE_SPLIT> {
    std::optional<std::vector<rank_t>> rank;
    std::vector<T> objective;
    std::vector<configuration_t> species;
    std::vector<cube_t<T>> sro;

    static sqs_result empty(std::vector<shell_weights_t<T>> const &weights,
                            std::vector<core::structure<T>> const &structures) {
      using namespace sqsgen::core::helpers;
      assert(structures.size() == weights.size());
      auto num_sl = structures.size();
      return {std::nullopt, as<std::vector>{}(range(num_sl) | views::transform([](auto) {
                                                return std::numeric_limits<T>::infinity();
                                              })),
              as<std::vector>{}(structures | views::transform([](auto s) { return s.species; })),
              as<std::vector>{}(range(num_sl) | views::transform([&](auto index) {
                                  return cube_t<T>(
                                      weights[index].size(), structures[index].num_species,
                                      structures[index].num_species, structures[index].num_species);
                                }))};
    }
  };

  template <class T, SublatticeMode Mode> using sqs_result_entry_t
      = std::tuple<T, std::vector<sqs_result<T, Mode>>>;

  namespace detail {

    static auto by_objective
        = [](auto &&lhs, auto &&rhs) { return std::get<0>(lhs) < std::get<0>(rhs); };

    template <class T, SublatticeMode Mode> struct sqs_result_entry_comparer {
      bool operator()(sqs_result_entry_t<T, Mode> &&lhs, sqs_result_entry_t<T, Mode> &&rhs) {
        return std::get<0>(lhs) < std::get<0>(rhs);
      }
    };

    template <class T, SublatticeMode Mode> using sqs_result_collection_base_t
        = core::helpers::sorted_vector<sqs_result_entry_t<T, Mode>, decltype(by_objective)>;
  }  // namespace detail

  template <class T, SublatticeMode Mode> class sqs_result_collection
      : public detail::sqs_result_collection_base_t<T, Mode> {
    using base = detail::sqs_result_collection_base_t<T, Mode>;
    std::mutex _mutex_insert;

  private:
    typename base::iterator where(sqs_result<T, Mode> const &result) {
      sqs_result_entry_t<T, Mode> entry
          = sqs_result_entry_t<T, Mode>{result.objective, std::vector<sqs_result<T, Mode>>{}};
      typename base::iterator i = std::lower_bound(this->begin(), this->end(), entry, this->cmp);
      return i == this->end() || this->cmp(entry, *i) ? this->end() : i;
    }

  public:
    typename base::iterator insert_result(sqs_result<T, Mode> &&result) {
      // this method migh be called from more threads simultaneously
      sqs_result_entry_t<T, Mode> entry
          = {result.objective, std::vector<sqs_result<T, Mode>>{result}};
      std::scoped_lock lock{_mutex_insert};
      typename base::iterator location = where(result);
      if (location != this->end()) {
        assert(std::get<0>(*location) == result.objective);
        std::get<1>(*location).push_back(result);
        return location;
      }

      return this->insert(entry);
    }

    [[nodiscard]] std::size_t num_results() const {
      std::size_t size{0};
      for (auto &&[_, collection] : this->_values) size += collection.size();
      return size;
    }
  };
}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_COLLECTION_H
