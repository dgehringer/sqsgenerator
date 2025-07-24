//
// Created by Dominik Gehringer on 02.03.25.
//

#ifndef SQSGEN_CORE_RESULTS_H
#define SQSGEN_CORE_RESULTS_H

#include <mutex>
#include <shared_mutex>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "sqsgen/core/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/optimization_config.h"
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

  }  // namespace detail

  template <class T, SublatticeMode Mode> using sqs_result_pack_data_t
      = helpers::sorted_vector<sqs_result_entry_t<T, Mode>, decltype(core::detail::by_objective)>;

  template <class T, SublatticeMode Mode> class sqs_result_collection {
    using pack_data_t = sqs_result_pack_data_t<T, Mode>;

  public:
    bool insert(sqs_result<T, Mode> &&result) ABSL_LOCKS_EXCLUDED(mutex_) {
      std::unique_lock lock(mutex_);
      if (auto it = data_.find(result.objective); it != data_.end())
        return it->second.insert(std::move(result)).second;
      data_.emplace(result.objective, absl::flat_hash_set<sqs_result<T, Mode>>{std::move(result)});
      objectives_.insert(result.objective);
      return true;  // new objective
    }

    auto front() const {
      std::shared_lock lock(mutex_);
      if (objectives_.empty())
        throw std::out_of_range("No results available");
      else
        return *data_.at(objectives_.front()).begin();
    }

    auto results_for_objective(T objective) {
      std::shared_lock lock(mutex_);
      if (auto it = data_.find(objective); it != data_.end())
        return it->second.size();
      else
        return std::size_t{0};
    }

    [[nodiscard]] std::size_t num_results() const {
      std::shared_lock lock(mutex_);
      std::size_t size = 0;
      for (auto const &pair : this->data_) size += pair.second.size();
      return size;
    }

    [[nodiscard]] T nth_best(std::size_t n) {
      std::shared_lock lock(mutex_);
      if (n >= objectives_.size()) return std::numeric_limits<T>::infinity();
      return objectives_.at(n);
    }

    pack_data_t results() {
      pack_data_t results(std::vector<sqs_result_entry_t<T, Mode>>{});
      std::shared_lock lock(mutex_);
      results.reserve(data_.size());
      for (auto const &[objective, collection] : data_)
        results.insert(
            {objective, std::vector<sqs_result<T, Mode>>(collection.begin(), collection.end())});
      return results;
    }
    pack_data_t remove_duplicates() { return results(); }

  private:
    mutable std::shared_mutex mutex_;
    helpers::sorted_vector<T> objectives_;
    absl::flat_hash_map<T, absl::flat_hash_set<sqs_result<T, Mode>>> data_;
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
      std::shared_ptr<core::structure<T>> _structure;
      opt_config_t<T, SUBLATTICE_MODE_INTERACT> _opt_config;

    public:
      explicit sqs_result_wrapper(sqs_result<T, SUBLATTICE_MODE_INTERACT> &&result,
                                  std::shared_ptr<core::structure<T>> structure,
                                  opt_config_t<T, SUBLATTICE_MODE_INTERACT> opt_config)
          : sqs_result<T, SUBLATTICE_MODE_INTERACT>(std::move(result)),
            _structure(structure),
            _opt_config(opt_config) {}

      core::structure<T> structure() {
        return core::structure<T>{_opt_config->structure.lattice,
                                  _opt_config->structure.frac_coords, this->species,
                                  _opt_config->structure.pbc};
      }

      configuration_t configuration() { return this->species; }

      std::string rank() {
        return core::rank_permutation(
                   as<std::vector>{}(this->species | views::transform([&](auto &&s) {
                                       return this->_opt_config->species_map.first.at(s);
                                     })))
            .str();
      }

      sro_parameter<T> parameter(usize_t shell, std::string const &i, std::string const &j) {
        if (!SYMBOL_MAP.contains(i))
          throw std::domain_error(format_string("Unknown atomic species \"%s\"", i));
        if (!SYMBOL_MAP.contains(j))
          throw std::domain_error(format_string("Unknown atomic species \"%s\"", j));
        return parameter(shell, SYMBOL_MAP.at(i), SYMBOL_MAP.at(j));
      }

      sro_parameter<T> parameter(usize_t shell, specie_t i, specie_t j) {
        auto shell_index = this->shell_index(shell);
        if (shell_index.has_value()) {
          auto ii = species_index(i);
          if (ii.has_value()) {
            auto jj = species_index(j);
            if (jj.has_value()) {
              return {shell, i, j, this->sro(shell_index.value(), ii.value(), jj.value())};
            }
            throw std::domain_error(format_string("This result does not contain species Z=%i", j));
          } else
            throw std::domain_error(format_string("This result does not contain Z=%i", i));
        } else
          throw std::domain_error(format_string("This result does not contain a shell %i", shell));
      }

      std::vector<sro_parameter<T>> parameter(usize_t shell) {
        const configuration_t s
            = as<std::vector>{}(views::elements<0>(this->_opt_config->species_map.first));
        std::vector<sro_parameter<T>> result;
        for (int i = 0; i < s.size(); ++i)
          for (int j = i; j < s.size(); ++j) result.push_back(parameter(shell, s[i], s[j]));
        return result;
      }

      std::vector<sro_parameter<T>> parameter(auto i, auto j) {
        return as<std::vector>{}(views::elements<0>(this->_opt_config->shell_map.first)
                                 | views::transform([&](auto &&s) { return parameter(s, i, j); }));
      }

      cube_t<T> parameter() { return this->sro; }

      std::optional<usize_t> shell_index(usize_t shell) {
        auto result = this->_opt_config->shell_map.first.find(shell);
        if (result != this->_opt_config->shell_map.first.end()) return result->second;
        return std::nullopt;
      }

      std::optional<specie_t> species_index(specie_t shell) {
        auto result = this->_opt_config->species_map.first.find(shell);
        if (result != this->_opt_config->species_map.first.end()) return result->second;
        return std::nullopt;
      }

      friend class sqs_result_wrapper<T, SUBLATTICE_MODE_SPLIT>;
      friend class sqs_result<T, SUBLATTICE_MODE_SPLIT>;

    private:
      auto sublattices() { return _opt_config->sublattice; }
    };

    template <class T> class sqs_result_wrapper<T, SUBLATTICE_MODE_SPLIT>
        : public sqs_result<T, SUBLATTICE_MODE_SPLIT> {
      std::shared_ptr<core::structure<T>> _structure;
      opt_config_t<T, SUBLATTICE_MODE_SPLIT> _opt_config;

    public:
      std::vector<sqs_result_wrapper<T, SUBLATTICE_MODE_INTERACT>> sublattices;
      explicit sqs_result_wrapper(sqs_result<T, SUBLATTICE_MODE_SPLIT> &&result,
                                  std::shared_ptr<core::structure<T>> structure,
                                  opt_config_t<T, SUBLATTICE_MODE_SPLIT> opt_config)
          : sqs_result<T, SUBLATTICE_MODE_SPLIT>(std::move(result)),
            _structure(structure),
            _opt_config(opt_config)

      {
        sublattices = make_sublattice_results(
            std::move(sqs_result<T, SUBLATTICE_MODE_SPLIT>::sublattices), structure, opt_config);
      }

      configuration_t configuration() {
        configuration_t new_species(_structure->species);
        for (auto &&sublattice : sublattices) {
          auto sls = sublattice.sublattices();
          if (sls.size() != 1)
            throw std::out_of_range(
                "a split mode result must have exactly one sublattice. Sublattices cannot contain "
                "sublattices themselves");
          auto index{0};
          for (auto site_index : sls.front().sites)
            new_species[site_index] = sublattice.species[index++];
        }
        return new_species;
      }

      auto structure() {
        return core::structure<T>{_structure->lattice, _structure->frac_coords, configuration(),
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

    template <class T, SublatticeMode Mode> using sqs_result_pack_wrapper_entry_t
        = std::tuple<T, std::vector<sqs_result_wrapper<T, Mode>>>;

    template <class T, SublatticeMode Mode> using sqs_result_pack_wrapper_data_t
        = sorted_vector<sqs_result_pack_wrapper_entry_t<T, Mode>,
                        decltype(core::detail::by_objective)>;

    template <class T, SublatticeMode Mode>
    sqs_result_pack_wrapper_data_t<T, Mode> from_result_collection(
        sqs_result_pack_data_t<T, Mode> &&results, std::shared_ptr<structure<T>> structure,
        opt_config_t<T, Mode> const &opt_config) {
      sqs_result_pack_wrapper_data_t<T, Mode> converted;
      converted.reserve(results.size());
      for (auto &&[objective, collection] : results) {
        std::vector<sqs_result_wrapper<T, Mode>> converted_collection;
        converted_collection.reserve(collection.size());
        while (collection.size() > 0) {
          converted_collection.push_back(
              sqs_result_wrapper<T, Mode>{std::move(collection.back()), structure, opt_config});
          collection.pop_back();
        }
        converted.insert(sqs_result_pack_wrapper_entry_t<T, Mode>{objective, converted_collection});
      }
      return converted;
    }

    template <class T, SublatticeMode Mode>
    opt_config_arg_t<T, Mode> opt_config_from_config(configuration<T> &&config) {
      auto result = as<std::vector>{}(
          optimization_config<T, Mode>::from_config(std::forward<configuration<T>>(config))
          | views::transform([&](auto &&c) { return c.data(); }));

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
    using pack_raw_data_t = sqs_result_pack_data_t<T, SMode>;
    using pack_data_t = core::detail::sqs_result_pack_wrapper_data_t<T, SMode>;

  public:
    configuration<T> config;

  private:
    std::shared_ptr<structure<T>> _structure;
    core::detail::opt_config_t<T, SMode> _optimization_config;

  public:
    sqs_statistics_data<T> statistics;
    pack_data_t results;

  public:
    typedef typename pack_data_t::iterator iterator;
    typedef typename pack_data_t::const_iterator const_iterator;
    iterator begin() { return results.begin(); }
    iterator end() { return results.end(); }
    const_iterator begin() const { return results.begin(); }
    const_iterator end() const { return results.end(); }

    auto size() { return results.size(); }

    auto num_results() {
      return helpers::sum(results | views::values
                          | views::transform([&](auto &c) { return c.size(); }));
    }

    sqs_result_pack() = default;

    sqs_result_pack(configuration<T> &&configuration,
                    core::detail::opt_config_arg_t<T, SMode> &&opt_config,
                    pack_raw_data_t &&results, sqs_statistics_data<T> &&stats)
        : statistics(stats),
          config(std::forward<core::configuration<T>>(configuration)),
          _optimization_config(core::detail::from_opt_config<T, SMode>(
              std::forward<detail::opt_config_arg_t<T, SMode>>(opt_config))),
          _structure(std::make_shared<structure<T>>(config.structure.structure())),
          results(core::detail::from_result_collection(std::forward<decltype(results)>(results),
                                                       _structure, _optimization_config)) {}

    sqs_result_pack(configuration<T> &&configuration, pack_raw_data_t &&results,
                    sqs_statistics_data<T> &&stats)
        : statistics(stats),
          config(std::forward<core::configuration<T>>(configuration)),
          _optimization_config(core::detail::from_opt_config<T, SMode>(
              core::detail::opt_config_from_config<T, SMode>(
                  std::forward<decltype(config)>(config)))),
          _structure(std::make_shared<structure<T>>(config.structure.structure())),
          results(core::detail::from_result_collection(std::forward<decltype(results)>(results),
                                                       _structure, _optimization_config)) {}
  };

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_RESULTS_H
