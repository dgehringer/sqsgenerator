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
    template <class T, SublatticeMode Mode> using opt_config_t
        = std::conditional_t<Mode == SUBLATTICE_MODE_SPLIT,
                             std::vector<std::shared_ptr<optimization_config_data<T>>>,
                             std::shared_ptr<optimization_config_data<T>>>;

    template <class T, SublatticeMode Mode> using opt_config_arg_t
        = std::conditional_t<Mode == SUBLATTICE_MODE_SPLIT,
                             std::vector<optimization_config_data<T>>, optimization_config_data<T>>;

    template <class, SublatticeMode> class sqs_result_wrapper {};

    template <class T> class sqs_result_wrapper<T, SUBLATTICE_MODE_INTERACT>
        : public sqs_result<T, SUBLATTICE_MODE_INTERACT> {
      std::shared_ptr<structure<T>> _structure;
      opt_config_t<T, SUBLATTICE_MODE_INTERACT> _opt_config;

    public:
      explicit sqs_result_wrapper(sqs_result<T, SUBLATTICE_MODE_INTERACT> &&result,
                                  std::shared_ptr<structure<T>> structure,
                                  opt_config_t<T, SUBLATTICE_MODE_INTERACT> opt_config)
          : sqs_result<T, SUBLATTICE_MODE_INTERACT>(std::move(result)),
            _structure(structure),
            _opt_config(opt_config) {}

      structure<T> structure() { return _opt_config->structure.apply_species(this->species); }

      auto sites() { return _opt_config->sites; }

      std::string rank(int base = 10) {
        return core::rank_permutation(this->_opt_config->species_packed).to_string(base);
      }
    };

    template <class T> class sqs_result_wrapper<T, SUBLATTICE_MODE_SPLIT>
        : public sqs_result<T, SUBLATTICE_MODE_SPLIT> {
      std::shared_ptr<structure<T>> _structure;
      opt_config_t<T, SUBLATTICE_MODE_SPLIT> _opt_config;

      std::vector<sqs_result_wrapper<T, SUBLATTICE_MODE_INTERACT>> _sublattice_results;

    public:
      explicit sqs_result_wrapper(sqs_result<T, SUBLATTICE_MODE_SPLIT> &&result,
                                  std::shared_ptr<structure<T>> structure,
                                  opt_config_t<T, SUBLATTICE_MODE_SPLIT> opt_config)
          : sqs_result<T, SUBLATTICE_MODE_SPLIT>(std::move(result)),
            _structure(structure),
            _opt_config(opt_config),
            _sublattice_results(
                make_sublattice_results(std::move(result.sublattices), structure, opt_config)) {}

      auto sublattices() { return _sublattice_results; }

      auto structure() {
        configuration_t new_species(_structure->species);
        for (auto &&sublattice : _sublattice_results) {
          if (sublattice.species.size() != sublattice.sites().size())
            throw std::out_of_range(
                "number of sites of the sublattice does not match the number of sites");
          auto index{0};
          for (auto site_index : sublattice.sites())
            new_species[site_index] = sublattice.species[index++];
        }
        return core::structure<T>{_structure->lattice, _structure->frac_coords, new_species,
                                  _structure->pbc};
      }

    private:
      static auto make_sublattice_results(
          std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>> &&results,
          std::shared_ptr<core::structure<T>> structure,
          opt_config_t<T, SUBLATTICE_MODE_SPLIT> opt_config) {
        if (results.size() != opt_config.size())
          throw std::invalid_argument("invalid number of sublattices and optimization configs");

        return core::helpers::as<std::vector>{}(
            helpers::range(results.size()) | views::transform([&](auto &&index) {
              return sqs_result_wrapper<T, SUBLATTICE_MODE_INTERACT>(std::move(results[index]),
                                                                     structure, opt_config[index]);
            }));
      }
    };

    template <class T, SublatticeMode Mode> using sqs_result_pack_entry_t
        = std::tuple<T, std::vector<sqs_result_wrapper<T, Mode>>>;

    template <class T, SublatticeMode Mode> using sqs_result_pack_collection_t
        = helpers::sorted_vector<sqs_result_pack_entry_t<T, Mode>,
                                 decltype(sqsgen::core::detail::by_objective)>;

    template <class T, SublatticeMode Mode>
    sqs_result_pack_collection_t<T, Mode> from_result_collection(
        sqs_result_collection_base_t<T, Mode> &&results, std::shared_ptr<structure<T>> structure,
        opt_config_t<T, Mode> const &opt_config) {
      sqs_result_pack_collection_t<T, Mode> converted;
      converted.reserve(results.size());
      for (auto &&[objective, collection] : results) {
        std::vector<sqs_result_wrapper<T, Mode>> converted_collection;
        converted_collection.reserve(collection.size());
        while (collection.size() > 0) {
          converted_collection.push_back(
              sqs_result_wrapper<T, Mode>{std::move(collection.back()), structure, opt_config});
          collection.pop_back();
        }
        converted.insert(sqs_result_pack_entry_t<T, Mode>{objective, converted_collection});
      }
      return converted;
    }

    template <class T, SublatticeMode Mode>
    opt_config_arg_t<T, Mode> opt_config_from_config(configuration<T> &&config) {
      auto result = optimization_config_data<T>::from_config(std::move(config));
      if constexpr (Mode == SUBLATTICE_MODE_SPLIT)
        return result;
      else
        return result.front();
    }

    template <class T, SublatticeMode Mode>
    opt_config_t<T, Mode> from_opt_config(opt_config_arg_t<T, Mode> &&opt_config) {
      if constexpr (Mode == SUBLATTICE_MODE_SPLIT)
        return core::helpers::as<std::vector>{}(
            opt_config | views::transform([](auto &&config) {
              return std::make_shared<optimization_config_data<T>>(config);
            }));
      else
        return std::make_shared<optimization_config_data<T>>(opt_config);
    }

  }  // namespace detail

  template <class T, SublatticeMode SMode> class sqs_result_pack {
    using sqs_result_collection_t = detail::sqs_result_collection_base_t<T, SMode>;

  public:
    sqs_statistics_data<T> statistics;
    configuration<T> config;

  private:
    detail::opt_config_t<T, SMode> _optimization_config;
    std::shared_ptr<structure<T>> _structure;
    detail::sqs_result_pack_collection_t<T, SMode> _results;

  public:
    sqs_result_pack(configuration<T> const &configuration,
                    detail::opt_config_arg_t<T, SMode> const &opt_config,
                    sqs_result_collection_t const &results, sqs_statistics_data<T> const &stats)
        : statistics(stats),
          config(configuration),
          _optimization_config(detail::from_opt_config<T, SMode>(opt_config)),
          _structure(std::make_shared<structure<T>>(config.structure.structure())),
          _results(detail::from_result_collection(results, _structure, _optimization_config)) {}

    sqs_result_pack(configuration<T> &&configuration,
                    detail::opt_config_arg_t<T, SMode> &&opt_config,
                    sqs_result_collection_t &&results, sqs_statistics_data<T> &&stats)
        : statistics(stats),
          config(std::move(configuration)),
          _optimization_config(detail::from_opt_config<T, SMode>(std::move(opt_config))),
          _structure(std::make_shared<structure<T>>(config.structure.structure())),
          _results(detail::from_result_collection(std::move(results), _structure,
                                                  _optimization_config)) {}

    sqs_result_pack(configuration<T> &&configuration, sqs_result_collection_t &&results,
                    sqs_statistics_data<T> &&stats)
        : statistics(stats),
          config(std::move(configuration)),
          _optimization_config(detail::from_opt_config<T, SMode>()),
          _structure(std::make_shared<structure<T>>(config.structure.structure())),
          _results(detail::from_result_collection(std::move(results), _structure,
                                                  _optimization_config)) {}
  };

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_RESULTS_H
