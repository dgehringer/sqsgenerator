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
#include "sqsgen/io/mpi.h"
#include "sqsgen/optimization/collection.h"
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
#ifdef WITH_MPI
    mpl::communicator comm;
#endif
    std::atomic<T> _best_objective;
    thread_config_t _thread_config;
    std::map<std::thread::id, int> _thread_map;

  protected:
    configuration<T> config;
    sqs_result_collection<T, Mode> results;
    std::vector<detail::optimization_config<T, Mode>> opt_configs;

    int thread_id() {
      auto this_thread_id = std::this_thread::get_id();
      if (!_thread_map.contains(this_thread_id))
        _thread_map.emplace(this_thread_id, _thread_map.size());
      return _thread_map[this_thread_id];
    }

    auto rank() { return comm.rank(); }

    int num_ranks() {
      if constexpr (io::mpi::HAVE_MPI)
        return comm.size();
      else
        return 1;
    }

    auto is_head() {
      if constexpr (io::mpi::HAVE_MPI)
        return comm.rank() == io::mpi::RANK_HEAD;
      else
        return true;
    }

    void barrier() {
      if constexpr (io::mpi::HAVE_MPI) comm.barrier();
    }

    T best_objective() { return _best_objective.load(); }

    void update_best_objective(T objective) {
      if (objective < _best_objective.load()) _best_objective.store(objective);
    }

    [[nodiscard]] usize_t num_threads() {
      if (std::holds_alternative<usize_t>(_thread_config)) {
        auto num_threads = std::get<usize_t>(_thread_config);
        return num_threads > 0 ? num_threads : std::thread::hardware_concurrency();
      } else {
        auto threads_per_rank = std::get<std::vector<usize_t>>(_thread_config);
        if (threads_per_rank.size() != num_ranks())
          throw std::invalid_argument(std::format(
              "The communicator has {} ranks, but the thread configuration contains {} entries",
              num_ranks(), threads_per_rank.size()));
        return threads_per_rank[rank()] > 0 ? threads_per_rank[rank()]
                                            : std::thread::hardware_concurrency();
      }
    }

    bounds_t<rank_t> iteration_range() {
      rank_t iterations{config.iterations.value()};
      auto r = rank();
      auto num_ranks = this->num_ranks();
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

    explicit optimizer_base(configuration<T>&& config,
#ifdef WITH_MPI
                            mpl::communicator comm = mpl::environment::comm_world()
#endif
                                )
        : config(config),
          _best_objective(std::numeric_limits<T>::max()),
          results(),
#ifdef WITH_MPI
          comm(comm),
#endif
          _thread_config(config.thread_config),
          opt_configs(detail::optimization_config<T, Mode>::from_config(config)) {
    }

    void insert_result(sqs_result<T, Mode>&& result) {
      results.insert_result(std::forward<sqs_result<T, Mode>>(result));
    }

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
      spdlog::set_level(spdlog::level::info);

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

      const auto pull_best_objective = [&] {
        // pull in the latest objective
        if constexpr (io::mpi::HAVE_MPI)
          if (mpi_mode && !head) {
            tick<TIMING_COMM> tick_comm;
            io::mpi::recv_all<sqsgen::objective<T>>(
                this->comm, sqsgen::objective<T>{},
                [&](auto&& o, auto) { this->update_best_objective(o.value); }, io::mpi::RANK_HEAD);
            statistics.tock(tick_comm);
          }
      };

      const auto worker = [&, stop = stop_source.get_token()](rank_t rstart, rank_t rend) {
        tick<TIMING_TOTAL> tick_total;
        spdlog::debug("[Rank {}, Thread {}] received chunk start={}, end={}", this->rank(),
                      this->thread_id(), rstart.to_string(), rend.to_string());
        if constexpr (io::mpi::HAVE_MPI)
          if (mpi_mode && rstart == start) {
            tick<TIMING_COMM> tick_comm;
            io::mpi::send(this->comm, io::mpi::rank_state{true}, io::mpi::RANK_HEAD);
            statistics.tock(tick_comm);
            this->barrier();
          }

        tick<TIMING_CHUNK_SETUP> tick_setup;
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

        statistics.tock(tick_setup);

        pull_best_objective();

        tick<TIMING_LOOP> tick_loop;
        for (auto i = rstart; i < rend; ++i) {
          if (stop.stop_requested()) {
            spdlog::info("[Rank {}, Thread {}] received stop signal ...", this->rank(),
                         this->thread_id());
            break;
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
          if (objective_value <= this->best_objective()) {
            // pull in changes from other ranks. Has another rank found a better
            this->update_best_objective(objective_value);
            sqs_result<T, SMode> current(objective_value, objective, species, sro);
            if constexpr (io::mpi::HAVE_MPI) {
              if (!head && mpi_mode) {
                tick<TIMING_COMM> tick_comm;
                io::mpi::send(this->comm, std::forward<decltype(current)>(current),
                              io::mpi::RANK_HEAD);
                statistics.tock(tick_comm);
              } else
                this->insert_result(std::move(current));
            } else
              this->insert_result(std::move(current));

            statistics.log_result(iterations_t{rstart + i - start}, objective_value);
          }
        }
        statistics.tock(tick_loop);

        pull_best_objective();

        statistics.add_working(-iterations);
        statistics.add_finished(iterations);

        if constexpr (io::mpi::HAVE_MPI)
          if (mpi_mode && (rend == end || stop.stop_requested())) {
            tick<TIMING_COMM> tick_comm;
            io::mpi::send(this->comm, io::mpi::rank_state{false}, io::mpi::RANK_HEAD);
            statistics.tock(tick_comm);
            this->barrier();
          }

        spdlog::debug("[Rank {}, Thread {}] finished chunk start={}, end={}", this->rank(),
                      this->thread_id(), rstart.to_string(), rend.to_string());
        statistics.tock(tick_total);
      };

      BS::thread_pool pool(this->num_threads());

      const auto schedule_main_loop = [&] {
        iterations_t chunk_size = this->config.chunk_size;
        pool.detach_blocks(start, end, worker,
                           static_cast<std::size_t>((end - start) / chunk_size));
        pool.wait();
      };
      if constexpr (io::mpi::HAVE_MPI) {
        if (head && mpi_mode) {
          std::size_t num_ranks{static_cast<std::size_t>(this->num_ranks())};
          std::vector<std::optional<io::mpi::rank_state>> rank_states{num_ranks};
          // there can be at most 2 * num_ranks requests in this pool, we handle that without any
          // additional complexity

          auto comm_thread = std::jthread([&] {
            spdlog::info("[Rank {}, COMM] started comm thread ...", io::mpi::RANK_HEAD);
            auto result_buffer = this->make_empty_result();
            while (!ranges::all_of(rank_states, [&](auto&& s) { return s.has_value(); })) {
              io::mpi::recv_all(this->comm, io::mpi::rank_state{}, [&](auto&& state, auto&& rank) {
                spdlog::debug(
                    "[Rank {}, COMM] sucessfully registered rank {} (rank_state[running={}])",
                    io::mpi::RANK_HEAD, rank, state.running);
                rank_states[rank] = std::make_optional(state);
              });
            }
            spdlog::info("[Rank {}, COMM] all ranks up and running ...", io::mpi::RANK_HEAD);
            while (ranges::any_of(rank_states, [&](auto&& s) { return s.value().running; })) {
              auto buffer = this->make_empty_result();
              T objective_to_distribute{std::numeric_limits<T>::max()};
              mpl::irequest_pool pool_objectives;

              io::mpi::recv_all<sqs_result<T, SMode>>(
                  this->comm, std::forward<decltype(buffer)>(buffer),
                  [&](auto&& result, auto&& rank) {
                    auto o = result.objective;
                    if (o <= this->best_objective()) {
                      spdlog::debug("[Rank {}, COMM] recieved result from rank {} (objective={}) ",
                                    io::mpi::RANK_HEAD, rank, o);
                      this->insert_result(std::forward<decltype(result)>(result));
                      if (o < this->best_objective()) {
                        this->update_best_objective(o);
                        objective_to_distribute = o;
                        spdlog::debug(
                            "[Rank {}, COMM] recieved lower objective from rank {} (objective={}) ",
                            io::mpi::RANK_HEAD, rank, o);
                        for (auto other : range(this->num_ranks()))
                          if (other != io::mpi::RANK_HEAD && other != rank)
                            pool_objectives.push(
                                this->comm.isend(objective_to_distribute, other,
                                                 mpl::tag_t(io::mpi::TAG_OBJECTIVE)));
                      }
                    }
                  });
              io::mpi::recv_all(this->comm, io::mpi::rank_state{}, [&](auto&& state, auto&& index) {
                spdlog::debug(
                    "[Rank {}, COMM] sucessfully unregistered rank {} (rank_state[running={}])",
                    io::mpi::RANK_HEAD, index, state.running);
                rank_states[index] = std::make_optional(state);
              });
              if (!pool_objectives.empty()) pool_objectives.waitall();
            }
            spdlog::info("[Rank {}, COMM] comm thread finished ...", io::mpi::RANK_HEAD);
          });

          schedule_main_loop();
          comm_thread.join();
        } else
          schedule_main_loop();
      } else
        schedule_main_loop();

      this->barrier();

      /*
       * A barrier is needed before the head rank pulls in the latest result. In case the head
       * rank would finish first we would enter have race condition leading to MPI failure
       */

      if (this->results.size()) {
        spdlog::info("[Rank {}] best_objective={}", this->rank(),
                     std::get<0>(this->results.front()));
        spdlog::info("[Rank {}] num_best_solutions={}", this->rank(),
                     std::get<1>(this->results.front()).size());
        spdlog::info("[Rank {}] num_solutions={}", this->rank(), this->results.num_results());
      }

      auto d = statistics.data();
      spdlog::info("[Rank {}] best_objective={}", this->rank(), d.best_objective);
      spdlog::info("[Rank {}] best_rank={}", this->rank(), d.best_rank);
      for (auto&& [timing, label] :
           std::map<Timing, std::string>{{TIMING_TOTAL, "total"},
                                         {TIMING_CHUNK_SETUP, "chunk_setup"},
                                         {TIMING_LOOP, "loop"},
                                         {TIMING_COMM, "comm"}}) {
        auto time_in_ns = static_cast<double>(d.timings.at(timing));
        auto total_time_in_ns = static_cast<double>(d.timings.at(TIMING_TOTAL));
        spdlog::info("[Rank {}] {} time ns={}, ns_per_iteration={:.1f}, relative={:.1f}%", this->rank(),
                     label, time_in_ns, time_in_ns / static_cast<double>(d.finished),
                     time_in_ns / total_time_in_ns * 100);
      }

      // compute the rank of each permutation, and reorder in case of interact mode
      if (this->is_head())
        for (auto& [_, results] : this->results)
          for (auto& result : results)  // use reference here, otherwise the reference gets moved
            detail::postprocess_results(result, this->opt_configs);

      return this->results.results();
    }
  };
}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_SQS_H
