//
// Created by Dominik Gehringer on 02.03.25.
//

#ifndef SQSGEN_CORE_RESULTS_H
#define SQSGEN_CORE_RESULTS_H
#include "optimization_config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  namespace ranges = std::ranges;
  namespace views = std::views;

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
        = helpers::sorted_vector<sqs_result_entry_t<T, Mode>, decltype(by_objective)>;
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

    auto front() const { return this->_values.front(); }

    base results() { return *this; }

    base remove_duplicates() {
      sqs_result_collection filtered;
      for (auto &&[_, collection] : this->_values) {
        helpers::sorted_vector<configuration_t> unique_species;
        for (auto &result : collection) {
          configuration_t conf;
          if constexpr (Mode == SUBLATTICE_MODE_INTERACT)
            conf = result.species;
          else
            for (auto &&sublattice : result.sublattices)
              conf.insert(conf.end(), sublattice.species.begin(), sublattice.species.end());
          if (!unique_species.contains(conf)) {
            unique_species.insert(conf);
            filtered.insert_result(std::forward<sqs_result<T, Mode>>(result));
          }
        }
      }
      return filtered.results();
    }

    [[nodiscard]] std::size_t num_results() const {
      std::size_t size{0};
      for (auto &&[_, collection] : this->_values) size += collection.size();
      return size;
    }
  };

  template <class, SublatticeMode> struct sqs_result_factory;

  template <class T> struct sqs_result_factory<T, SUBLATTICE_MODE_INTERACT> {
    static sqs_result<T, SUBLATTICE_MODE_INTERACT> empty(auto num_atoms, auto num_shells,
                                                         auto num_species) {
      return {std::numeric_limits<T>::infinity(), configuration_t(num_atoms),
              cube_t<T>(num_shells, num_species, num_species)};
    }
  };

  template <class T> struct sqs_result_factory<T, SUBLATTICE_MODE_SPLIT> {
    template <ranges::range RNumAtoms, ranges::range RNumShells, ranges::range RNumSpecies>
    static sqs_result<T, SUBLATTICE_MODE_SPLIT> empty(RNumAtoms &&range_num_atoms,
                                                      RNumShells &&range_num_shells,
                                                      RNumSpecies &&range_num_species) {
      using namespace helpers;
      auto num_atoms = as<std::vector>{}(range_num_atoms);
      auto num_shells = as<std::vector>{}(range_num_shells);
      auto num_species = as<std::vector>{}(range_num_species);
      if (num_species.size() != num_shells.size() || num_species.size() != num_atoms.size())
        throw std::invalid_argument("invalid sizes");
      return {std::numeric_limits<T>::infinity(),
              as<std::vector>{}(range(num_atoms.size()) | views::transform([&](auto &&index) {
                                  return sqs_result_factory<T, SUBLATTICE_MODE_INTERACT>::empty(
                                      num_atoms[index], num_shells[index], num_species[index]);
                                }))};
    }
  };

  namespace detail {
    template <class, SublatticeMode> class sqs_result_wrapper {};

    template <class T> class sqs_result_wrapper<T, SUBLATTICE_MODE_INTERACT> {
      std::shared_ptr<structure<T>> _structure;

    public:
      sqs_result<T, SUBLATTICE_MODE_INTERACT> raw;

      explicit sqs_result_wrapper(sqs_result<T, SUBLATTICE_MODE_INTERACT> &&result,
                                  std::shared_ptr<structure<T>> structure)
          : raw(std::move(result)), _structure(structure) {}
    };

  }  // namespace detail

  template <class T, SublatticeMode SMode> class sqs_result_pack {
    using opt_config_t
        = std::conditional_t<SMode == SUBLATTICE_MODE_INTERACT, optimization_config_data<T>,
                             std::vector<optimization_config_data<T>>>;
    using collection_t = detail::sqs_result_collection_base_t<T, SMode>;
    std::shared_ptr<configuration<T>> _config;
    std::shared_ptr<opt_config_t> _optimization_config;
    std::shared_ptr<collection_t> _results;
    sqs_statistics_data<T> _statistics;

  public:
    sqs_result_pack(configuration<T> const &configuration, opt_config_t const &opt_config,
                    collection_t const &results, sqs_statistics_data<T> const &statistics)
        : _config(std::make_shared<core::configuration<T>>(configuration)),
          _optimization_config(std::make_shared<opt_config_t>(opt_config)),
          _results(std::make_shared<collection_t>(results)),
          _statistics(statistics) {}

    sqs_result_pack(configuration<T> &&configuration, opt_config_t &&opt_config,
                    collection_t &&results, sqs_statistics_data<T> &&statistics)
        : _config(std::make_shared<core::configuration<T>>(std::move(configuration))),
          _optimization_config(std::make_shared<opt_config_t>(std::move(opt_config))),
          _results(std::make_shared<collection_t>(std::move(results))),
          _statistics(std::move(statistics)) {}

    [[nodiscard]] configuration<T> const &config() const { return *_config; }
    [[nodiscard]] opt_config_t const &optimization_config() const { return *_optimization_config; }
    [[nodiscard]] sqs_statistics_data<T> const &statistics() const { return _statistics; }
  };

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_RESULTS_H
