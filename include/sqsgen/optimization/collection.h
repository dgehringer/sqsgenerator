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

    sqs_result(T objective, configuration_t species, cube_t<T> sro)
        : rank(std::nullopt),
          objective(objective),
          species(std::move(species)),
          sro(std::move(sro)) {}
    // compatibility constructor to
    sqs_result(T, T objective, configuration_t species, cube_t<T> sro)
        : sqs_result(objective, species, std::move(sro)) {}

    static sqs_result empty(shell_weights_t<T> const &weights,
                            core::structure<T> const &structure) {
      return {std::numeric_limits<T>::infinity(), configuration_t(structure.species.size()),
              cube_t<T>(weights.size(), structure.num_species, structure.num_species)};
    }
  };

  template <class T> struct sqs_result<T, SUBLATTICE_MODE_SPLIT> {
    T objective;
    std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>> sublattices;

    sqs_result(T objective, std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>> const &sublattices)
        : objective(objective), sublattices(sublattices) {}

    sqs_result(T objective, std::vector<T> const &objectives,
               const std::vector<configuration_t> &species, std::vector<cube_t<T>> const &sro)
        : objective(objective) {
      if (objectives.size() != species.size() || objectives.size() != sro.size())
        throw std::invalid_argument("invalid number entries");
      sublattices.reserve(objectives.size());
      core::helpers::for_each(
          [&](auto &&index) {
            sublattices.push_back({objectives[index], species[index], sro[index]});
          },
          species.size());
    }

    static sqs_result empty(std::vector<shell_weights_t<T>> const &weights,
                            std::vector<core::structure<T>> const &structures) {
      using namespace sqsgen::core::helpers;
      assert(structures.size() == weights.size());
      return {std::numeric_limits<T>::infinity(),
              as<std::vector>{}(range(structures.size()) | views::transform([&](auto &&index) {
                                  return sqs_result<T, SUBLATTICE_MODE_INTERACT>::empty(
                                      weights[index], structures[index]);
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
