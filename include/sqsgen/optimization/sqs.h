//
// Created by Dominik Gehringer on 05.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_SQS_H
#define SQSGEN_OPTIMIZATION_SQS_H

#include <BS_thread_pool.hpp>

#include "spdlog/spdlog.h"
#include "sqsgen/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/shuffle.h"
#include "sqsgen/io/mpi/request_pool.h"
#include "sqsgen/optimization/collection.h"
#include "sqsgen/optimization/comm.h"
#include "sqsgen/optimization/helpers.h"
#include "sqsgen/types.h"
#include "statistics.h"

namespace sqsgen::optimization {

  namespace detail {
    using namespace core::helpers;
    namespace ranges = std::ranges;
    namespace views = ranges::views;

    template <ranges::sized_range... R> bool same_length(R&&... r) {
      if constexpr (sizeof...(R) == 0)
        return false;
      else if constexpr (sizeof...(R) == 1)
        return true;
      else {
        std::array<size_t, sizeof...(R)> lengths{ranges::size(r)...};
        auto first_length = std::get<0>(lengths);
        return ranges::all_of(lengths, [first_length](auto l) { return l == first_length; });
      }
    }

    template <class T, SublatticeMode Mode> struct optimization_config {
      core::structure<T> structure;
      core::structure<T> sorted;
      std::vector<bounds_t<usize_t>> bounds;
      std::vector<usize_t> sort_order;
      configuration_t species_packed;
      index_mapping_t<specie_t, specie_t> species_map;
      index_mapping_t<usize_t, usize_t> shell_map;
      std::vector<core::atom_pair<usize_t>> pairs;
      cube_t<T> pair_weights;
      cube_t<T> prefactors;
      cube_t<T> target_objective;
      std::vector<T> shell_radii;
      shell_weights_t<T> shell_weights;
      core::shuffler shuffler;

      static std::vector<optimization_config> from_config(configuration<T> config) {
        auto [structures, sorted, bounds, sort_order] = decompose_sort_and_bounds(config);
        if (!detail::same_length(structures, sorted, bounds, sort_order, config.shell_radii,
                                 config.shell_weights, config.prefactors, config.target_objective,
                                 config.pair_weights))
          throw std::invalid_argument("Incompatible configuration");
        auto num_sublattices = sorted.size();

        std::vector<optimization_config> configs;
        configs.reserve(num_sublattices);
        for (auto i = 0u; i < num_sublattices; i++) {
          auto [species_packed, species_map, species_rmap, pairs, shells_map, shells_rmap,
                pair_weights]
              = shared(sorted[i], config.shell_radii[i], config.shell_weights[i],
                       config.pair_weights[i]);
          // core::shuffler shuffler(bounds[i]);
          configs.emplace_back(
              optimization_config{std::move(structures[i]),
                                  std::move(sorted[i]),
                                  {bounds[i]},
                                  std::move(sort_order[i]),
                                  std::move(species_packed),
                                  std::make_pair(std::move(species_map), std::move(species_rmap)),
                                  std::make_pair(std::move(shells_map), std::move(shells_rmap)),
                                  std::move(pairs),
                                  std::move(config.pair_weights[i]),
                                  std::move(config.prefactors[i]),
                                  std::move(config.target_objective[i]),
                                  std::move(config.shell_radii[i]),
                                  std::move(config.shell_weights[i]),
                                  core::shuffler({bounds[i]})});
        }
        return configs;
      }

    private:
      static auto decompose_sort_and_bounds(configuration<T> const& config) {
        if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
          auto s = config.structure.structure().apply_composition(config.composition);

          auto [sorted, bounds, sort_order]
              = helpers::compute_shuffling_bounds(s, config.composition);

          return std::make_tuple(std::vector{s}, std::vector{sorted},
                                 stl_matrix_t<bounds_t<usize_t>>{bounds},
                                 stl_matrix_t<usize_t>{sort_order});
        } else if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
          auto structures
              = config.structure.structure().apply_composition_and_decompose(config.composition);
          return std::make_tuple(
              structures, structures,
              as<std::vector>{}(structures | views::transform([](auto&& s) -> bounds_t<usize_t> {
                                  return {0, s.size()};
                                })),
              as<std::vector>{}(structures | views::transform([](auto&& s) -> std::vector<usize_t> {
                                  return as<std::vector>{}(range(static_cast<usize_t>(s.size())));
                                })));
        }
      }
      static auto shared(core::structure<T> sorted, std::vector<T> const& radii,
                         shell_weights_t<T> const& weights, cube_t<T> const& pair_weights) {
        auto [species_map, species_rmap] = make_index_mapping<specie_t>(sorted.species);
        auto species_packed
            = as<std::vector>{}(sorted.species | views::transform([&species_map](auto&& s) {
                                  return species_map.at(s);
                                }));
        auto [pairs, shells_map, shells_rmap] = sorted.pairs(radii, weights);
        std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
          return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
        });
        return std::make_tuple(
            species_packed, species_map, species_rmap, pairs, shells_map, shells_rmap,
            helpers::scaled_pair_weights(pair_weights, weights, sorted.num_species));
      }
    };

    template <class T, SublatticeMode Mode> using lift_t
        = std::conditional_t<Mode == SUBLATTICE_MODE_INTERACT, T, std::vector<T>>;

    template <class T, SublatticeMode Mode>
    sqs_result<T, Mode> postprocess_results(sqs_result<T, Mode>& r,
                                            std::vector<optimization_config<T, Mode>>& configs) {
      const auto restore_order
          = [&](configuration_t& species,
                optimization_config<T, Mode> const& config) -> configuration_t {
        auto species_rmap = std::get<1>(config.species_map);
        return as<std::vector>{}(config.sort_order | views::transform([&](auto&& index) {
                                   return species_rmap[species[index]];
                                 }));
      };
      if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
        r.species = restore_order(r.species, configs.front());
        return r;
      }
      if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
        assert(configs.size() == r.sublattices.size());
        for_each([&](auto&& i) { restore_order(r.sublattices[i].species, configs[i]); },
                 range(configs.size()));
        return r;
      }
    }

  }  // namespace detail

  template <class T, SublatticeMode Mode> struct optimizer_base {
    configuration<T> config;
    std::atomic<T> _best_objective;
    sqs_result_collection<T, Mode> _results;
    comm::MPICommunicator<T> _comm;
    std::vector<detail::optimization_config<T, Mode>> opt_configs;

    auto rank() { return _comm.rank(); }

    auto num_ranks() { return _comm.num_ranks(); }

    auto is_head() { return _comm.is_head(); }

    void barrier() { _comm.barrier(); }
    auto num_threads() { return _comm.num_threads(); }

    bounds_t<rank_t> iteration_range() {
      rank_t iterations{config.iterations.value()};
      auto r = this->_comm.rank();
      auto num_ranks = _comm.num_ranks();
      auto offerr = iterations % num_ranks;
      auto rank_range = iterations / num_ranks;
      if (r < offerr) {
        return {r * (rank_range + 1), (r + 1) * (rank_range + 1)};
      } else {
        auto offset = offerr * (rank_range + 1);
        return {offset + (r - offerr) * rank_range, offset + (r - offerr + 1) * rank_range};
      }
    }

    template <class Fn>
    detail::lift_t<std::decay_t<std::invoke_result_t<Fn, detail::optimization_config<T, Mode>>>,
                   Mode>
    transpose_setting(Fn&& fn) {
      if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
        return fn(opt_configs.front());
      } else if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
        return core::helpers::as<std::vector>{}(
            opt_configs | std::ranges::views::transform([&](auto&& conf) { return fn(conf); }));
      }
      throw std::invalid_argument("unrecognized operation");
    }

    explicit optimizer_base(configuration<T>&& config)
        : config(config),
          _best_objective(std::numeric_limits<T>::max()),
          _results(),
          _comm(config.thread_config),
          opt_configs(detail::optimization_config<T, Mode>::from_config(config)) {}

    sqs_result<T, Mode> make_empty_result() {
      using namespace sqsgen::core::helpers;
      if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
        auto c = opt_configs.front();
        return sqs_result<T, Mode>::empty(c.structure.size(), c.shell_weights.size(),
                                          c.sorted.num_species);
      } else if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
        return sqs_result<T, Mode>::empty(
            opt_configs | views::transform([](auto&& c) { return c.species_packed.size(); }),
            opt_configs | views::transform([](auto&& c) { return c.shell_weights.size(); }),
            opt_configs | views::transform([](auto&& c) { return c.sorted.num_species; }));
      }
      throw std::invalid_argument("invalid lattice mode");
    }
  };

  template <class T, IterationMode IMode, SublatticeMode SMode> class optimizer
      : public optimizer_base<T, SMode> {
  public:
    explicit optimizer(configuration<T>&& config)
        : optimizer_base<T, SMode>(std::forward<configuration<T>>(config)) {}
    auto run() {
      using namespace sqsgen::core::helpers;
      spdlog::set_level(spdlog::level::debug);

      auto head = this->is_head();
      auto mpi_mode = this->num_ranks() > 1;
      if (mpi_mode && head) spdlog::info("[Rank {}] Running optimizer in MPI mode", this->rank());
      auto [start, end] = this->iteration_range();
      spdlog::info("[Rank {}] start={}, end={}", this->rank(), start.to_string(), end.to_string());

      auto num_sublattices = this->opt_configs.size();
      auto pairs{this->transpose_setting([](auto&& c) { return c.pairs; })};
      auto prefactors{this->transpose_setting([](auto&& c) { return c.prefactors; })};
      auto pair_weights{this->transpose_setting([](auto&& c) { return c.pair_weights; })};
      auto target_objective{this->transpose_setting([](auto&& c) { return c.target_objective; })};
      auto shuffler{this->transpose_setting([](auto&& c) { return c.shuffler; })};
      auto species_packed{this->transpose_setting([](auto&& c) { return c.species_packed; })};
      auto num_shells{this->transpose_setting([](auto&& c) { return c.shell_weights.size(); })};
      auto num_species{this->transpose_setting([](auto&& c) { return c.sorted.num_species; })};

      sqs_statistics<T> statistics;
      std::stop_source stop_source;


      const auto worker = [&, stop = stop_source.get_token()](rank_t rstart, rank_t rend) {
        spdlog::debug("[Rank {}, Thread {}] received chunk start={}, end={}", this->rank(), 0,
                      rstart.to_string(), rend.to_string());
        if (stop.stop_requested()) return;
        io::mpi::outbound_request_pool<sqs_result<T, SMode>> pool_results;
        iterations_t iterations{rend - rstart};

        auto bonds{this->transpose_setting([](auto&& c) {
          return cube_t<usize_t>(c.shell_weights.size(), c.sorted.num_species,
                                 c.sorted.num_species);
        })};
        auto sro{this->transpose_setting([](auto&& c) {
          return cube_t<T>(c.shell_weights.size(), c.sorted.num_species, c.sorted.num_species);
        })};
        auto objective = this->transpose_setting([](auto&&) { return T(0); });
        auto species{species_packed};
        statistics.add_working(iterations);

        if constexpr (SMode == SUBLATTICE_MODE_INTERACT && IMode == ITERATION_MODE_SYSTEMATIC)
          shuffler.unrank_permutation(species, rstart + 1);

        for (auto i = rstart; i < rend; ++i) {
          if (stop.stop_requested()) {
            spdlog::info("[Rank {}, Thread {}] received stop signal ...", this->rank(), 0);
            return;
          }
          if constexpr (SMode == SUBLATTICE_MODE_INTERACT) {
            if constexpr (IMode == ITERATION_MODE_SYSTEMATIC)
              assert(i + 1 == shuffler.rank_permutation(species));
            helpers::count_bonds(bonds, pairs, species);
            objective = helpers::compute_objective(sro, bonds, prefactors, pair_weights,
                                                   target_objective, num_shells, num_species);
            shuffler.template shuffle<IMode>(species);

          } else if constexpr (SMode == SUBLATTICE_MODE_SPLIT) {
            std::vector<T> objectives(num_sublattices);
            for (auto sigma = 0; sigma < num_sublattices; ++sigma) {
              shuffler.at(sigma).template shuffle<IMode>(species.at(sigma));
              helpers::count_bonds(bonds.at(sigma), pairs.at(sigma), species.at(sigma));
              objective.at(sigma) = helpers::compute_objective(
                  sro.at(sigma), bonds.at(sigma), prefactors.at(sigma), pair_weights.at(sigma),
                  target_objective.at(sigma), num_shells.at(sigma), num_species.at(sigma));
            }
          }
          T objective_value;
          if constexpr (SMode == SUBLATTICE_MODE_INTERACT) {
            objective_value = objective;
          } else if constexpr (SMode == SUBLATTICE_MODE_SPLIT) {
            objective_value = sum(objective);
          }
          // symmetrize bonds for each shell and compute objective function
          if (objective_value <= this->_best_objective.load()) {
            // pull in changes from other ranks. Has another rank found a better
            sqs_result<T, SMode> current(objective_value, objective, species, sro);
            if (!head) pool_results.send(std::forward<decltype(current)>(current), io::mpi::RANK_HEAD);
            this->_results.insert_result(std::move(current));
            statistics.log_result(iterations_t{rstart + i - start}, objective_value);
          }
        }
        statistics.add_working(-iterations);
        statistics.add_working(iterations);
        spdlog::debug("[Rank {}, Thread {}] finished chunk start={}, end={}", this->rank(), 0,
                      rstart.to_string(), rend.to_string());
      };

      iterations_t chunk_size = this->config.chunk_size;
      this->barrier();
      BS::thread_pool pool(this->num_threads());
      pool.detach_blocks(start, end, worker, static_cast<std::size_t>((end - start) / chunk_size));
      pool.wait();
      this->barrier();

      /*
       * A barrier is needed before the head rank pulls in the latest result. In case the head
       * rank would finish first we would enter have race condition leading to MPI failure
       */

      if (this->is_head()) {
        spdlog::info("[Rank {}] best_objective={}", this->rank(),
                     std::get<0>(this->_results.front()));
        spdlog::info("[Rank {}] num_best_solutions={}", this->rank(),
                     std::get<1>(this->_results.front()).size());
        spdlog::info("[Rank {}] num_solutions={}", this->rank(), this->_results.num_results());
      }

      // compute the rank of each permutation, and reorder in case of interact mode
      if (this->is_head())
        for (auto& [_, results] : this->_results)
          for (auto& result : results)  // use reference here, otherwise the reference gets moved
            detail::postprocess_results(result, this->opt_configs);

      return this->_results.results();
    }
  };
}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_SQS_H
