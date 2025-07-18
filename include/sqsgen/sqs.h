//
// Created by Dominik Gehringer on 05.01.25.
//

#ifndef SQSGEN_SQS_H
#define SQSGEN_SQS_H

#include <BS_thread_pool.hpp>
#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>

#include "sqsgen/core/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/optimization.h"
#include "sqsgen/core/optimization_config.h"
#include "sqsgen/core/results.h"
#include "sqsgen/core/shuffle.h"
#include "sqsgen/core/statistics.h"
#include "sqsgen/io/mpi.h"
#include "sqsgen/log.h"
#include "sqsgen/types.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <cstring>
#endif

namespace sqsgen {

  namespace signal {

    static std::atomic _interrupted{false};

    static bool interrupted() { return _interrupted.load(std::memory_order_relaxed); }

#ifdef _WIN32
    // Windows handler
    BOOL WINAPI console_handler(DWORD ctrl_type) {
      spdlog::warn(std::format("Interrupt oder termination signal received: {}", ctrl_type));
      switch (ctrl_type) {
        case CTRL_C_EVENT:
          _interrupted.store(true);
          return TRUE;
        case CTRL_CLOSE_EVENT:
          _interrupted.store(true);
          return TRUE;
        case CTRL_SHUTDOWN_EVENT:
          _interrupted.store(true);
          return TRUE;
        default:
          return FALSE;
      }
    }
#else
    // Unix-like handler
    static void signal_handler(int signal) {
      spdlog::warn(
          std::format("Interrupt oder termination signal received: {}", strsignal(signal)));
      _interrupted.store(true);
    }
#endif

    // Setup handlers
    static void setup_signal_handlers() {
#ifdef _WIN32
      SetConsoleCtrlHandler(console_handler, TRUE);
#else
      std::signal(SIGINT, signal_handler);
      std::signal(SIGTERM, signal_handler);
#endif
    }

    static void teardown_signal_handlers() {
#ifdef _WIN32
      SetConsoleCtrlHandler(nullptr, FALSE);  // Remove handler
#else
      std::signal(SIGINT, SIG_DFL);  // Restore default behavior
      std::signal(SIGTERM, SIG_DFL);
#endif
    }
  }  // namespace signal

  namespace detail {
    using namespace sqsgen::core;
    using namespace sqsgen::core::helpers;

    namespace ranges = std::ranges;
    namespace views = ranges::views;

    template <class T, SublatticeMode Mode> using lift_t
        = std::conditional_t<Mode == SUBLATTICE_MODE_INTERACT, T, std::vector<T>>;

    template <class T, SublatticeMode Mode>
    sqs_result<T, Mode> postprocess_results(sqs_result<T, Mode>& r,
                                            std::vector<optimization_config<T, Mode>>& configs) {
      const auto restore_order
          = [&](configuration_t& species,
                optimization_config<T, Mode> const& config) -> configuration_t {
        auto sorted = as<std::vector>{}(config.sort_order | views::transform([&](auto&& index) {
                                          return config.species_map.second.at(species[index]);
                                        }));
        configuration_t reversed(species.size());
        for (auto i = 0; i < species.size(); i++)
          reversed[config.sort_order[i]] = config.species_map.second.at(species[i]);

        return reversed;
      };
      if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
        r.species = restore_order(r.species, configs.front());
        return r;
      }
      if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
        assert(configs.size() == r.sublattices.size());
        for_each(
            [&](auto&& i) {
              r.sublattices[i].species = restore_order(r.sublattices[i].species, configs[i]);
            },
            range(configs.size()));
        return r;
      }
    }

    template <class T>
    void log_statistics(sqs_statistics_data<T>&& d, int rank, std::string const& info = "") {
      spdlog::info(std::format("[Rank {}] {} best_objective={}", rank, info, d.best_objective));
      spdlog::info(std::format("[Rank {}] {} best_rank={}", rank, info, d.best_rank));
      for (auto&& [timing, label] :
           std::map<Timing, std::string>{{TIMING_TOTAL, "total"},
                                         {TIMING_CHUNK_SETUP, "chunk_setup"},
                                         {TIMING_LOOP, "loop"},
                                         {TIMING_COMM, "comm"}}) {
        auto time_in_ns = static_cast<double>(d.timings.at(timing));
        auto total_time_in_ns = static_cast<double>(d.timings.at(TIMING_TOTAL));
        spdlog::info(
            std::format("[Rank {}] {} {} time ns={}, ns_per_iteration={:.1f}, relative={:.1f}%",
                        rank, info, label, time_in_ns, time_in_ns / static_cast<double>(d.finished),
                        time_in_ns / total_time_in_ns * 100));
      }
    }

  }  // namespace detail

  namespace optimization = sqsgen::core::optimization;

  template <class T, SublatticeMode Mode> struct optimizer_base {
#ifdef WITH_MPI
    mpl::communicator comm;
#endif
    std::atomic<T> _best_objective;
    std::atomic<T> _search_objective;
    thread_config_t _thread_config;
    std::map<std::thread::id, int> _thread_map;

  protected:
    core::configuration<T> config;
    core::sqs_result_collection<T, Mode> results;
    std::vector<core::optimization_config<T, Mode>> opt_configs;

    int thread_id() {
      auto this_thread_id = std::this_thread::get_id();
      if (!_thread_map.contains(this_thread_id))
        _thread_map.emplace(this_thread_id, _thread_map.size());
      return _thread_map[this_thread_id];
    }

    auto rank() {
#ifdef WITH_MPI
      return comm.rank();
#else
      return 0;
#endif
    }

    int num_ranks() {
#ifdef WITH_MPI
      return comm.size();
#else
      return 1;
#endif
    }

    auto is_head() {
#ifdef WITH_MPI
      return comm.rank() == io::mpi::RANK_HEAD;
#else
      return true;
#endif
    }

    void barrier() {
#ifdef WITH_MPI
      comm.barrier();
#endif
    }

    T best_objective() { return _best_objective.load(); }

    T search_objective() { return _search_objective.load(); }

    void update_objectives(objective<T>&& objective) {
      if (objective.best < _best_objective.load()) _best_objective.store(objective.best);
      if (objective.search < _search_objective.load()) _search_objective.store(objective.search);
    }

    void update_best_objective(T objective) {
      if (objective < _best_objective.load()) _best_objective.store(objective);
    }

    void update_search_objective(T objective) {
      if (objective < _search_objective.load()) _search_objective.store(objective);
    }

    [[nodiscard]] usize_t num_threads() {
      if (_thread_config.size() == 1) return _thread_config.front();
      if (_thread_config.size() != num_ranks())
        throw std::invalid_argument(std::format(
            "The communicator has {} ranks, but the thread configuration contains {} entries",
            num_ranks(), _thread_config.size()));
      return _thread_config[rank()] > 0 ? _thread_config[rank()]
                                        : std::thread::hardware_concurrency();
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

    template <class Fn> sqsgen::detail::lift_t<
        std::decay_t<std::invoke_result_t<Fn, core::optimization_config<T, Mode>>>, Mode>
    transpose_setting(Fn&& fn) {
      if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
        return fn(opt_configs.front());
      } else if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
        return core::helpers::as<std::vector>{}(
            opt_configs | std::ranges::views::transform([&](auto&& conf) { return fn(conf); }));
      }
      throw std::invalid_argument("unrecognized operation");
    }

    explicit optimizer_base(core::configuration<T>&& config
#ifdef WITH_MPI
                            ,
                            mpl::communicator comm = mpl::environment::comm_world()
#endif
                                )
        : config(config),
          _best_objective(std::numeric_limits<T>::max()),
          _search_objective(std::numeric_limits<T>::max()),
          results(),
#ifdef WITH_MPI
          comm(comm),
#endif
          _thread_config(config.thread_config),
          opt_configs(core::optimization_config<T, Mode>::from_config(config)) {
    }

    void insert_result(sqs_result<T, Mode>&& result) {
      results.insert_result(std::forward<sqs_result<T, Mode>>(result));
    }

    auto nth_best_objective(auto n) { return results.nth_best(n); }

    sqs_result<T, Mode> make_empty_result() {
      using namespace sqsgen::core::helpers;
      if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
        auto c = opt_configs.front();
        return core::sqs_result_factory<T, Mode>::empty(c.structure.size(), c.shell_weights.size(),
                                                        c.sorted.num_species);
      } else if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
        return core::sqs_result_factory<T, Mode>::empty(
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
    explicit optimizer(core::configuration<T>&& config)
        : optimizer_base<T, SMode>(std::forward<core::configuration<T>>(config)) {}
    auto run(spdlog::level::level_enum level = spdlog::level::info,
             std::optional<sqs_callback_t> callback = std::nullopt) {
      using namespace sqsgen::core::helpers;
      spdlog::set_level(level);

      auto head = this->is_head();
      auto mpi_mode = this->num_ranks() > 1;

      // if we are not in MPI mode we install the signal handlers
      if (!mpi_mode) signal::setup_signal_handlers();

      if (mpi_mode && head)
        spdlog::info(std::format("[Rank {}] Running optimizer in MPI mode", this->rank()));
      auto [start, end] = this->iteration_range();
      spdlog::info(std::format("[Rank {}] start={}, end={}", this->rank(), start.str(), end.str()));

      auto num_sublattices = this->opt_configs.size();
      auto pairs{this->transpose_setting([](auto&& c) { return c.pairs; })};
      auto prefactors{this->transpose_setting([](auto&& c) { return c.prefactors; })};
      auto pair_weights{this->transpose_setting([](auto&& c) { return c.pair_weights; })};
      auto target_objective{this->transpose_setting([](auto&& c) { return c.target_objective; })};
      auto shuffler{this->transpose_setting([](auto&& c) { return c.shuffler; })};
      auto species_packed{this->transpose_setting([](auto&& c) { return c.species_packed; })};
      auto num_shells{this->transpose_setting([](auto&& c) { return c.shell_weights.size(); })};
      auto num_species{this->transpose_setting([](auto&& c) { return c.sorted.num_species; })};

      auto keep = this->config.keep;

      core::sqs_statistics<T> statistics;
      auto stop_source = std::make_shared<std::stop_source>();

      const auto worker = [&, stop = stop_source->get_token()](rank_t rstart, rank_t rend) {
        if (stop.stop_requested()) return;
        core::tick<TIMING_TOTAL> tick_total;
        spdlog::debug(std::format("[Rank {}, Thread {}] received chunk start={}, end={}",
                                  this->rank(), this->thread_id(), rstart.str(), rend.str()));

        core::tick<TIMING_CHUNK_SETUP> tick_setup;
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
          shuffler.template unrank_permutation<ITERATION_MODE_SYSTEMATIC>(species, rstart + 1);

        statistics.tock(tick_setup);

        core::tick<TIMING_LOOP> tick_loop;

        for (auto i = rstart; i < rend; ++i) {
          if (!mpi_mode && signal::interrupted()) {
            spdlog::info(std::format("[Rank {}, Thread {}] Process received SIGTERM or SIGINT ...",
                                     this->rank(), this->thread_id()));
            break;
          }
          if (stop.stop_requested()) {
            spdlog::info(std::format("[Rank {}, Thread {}] received stop signal ...", this->rank(),
                                     this->thread_id()));
            break;
          }
          if constexpr (SMode == SUBLATTICE_MODE_INTERACT) {
            if constexpr (IMode == ITERATION_MODE_SYSTEMATIC)
              assert(i + 1 == shuffler.rank_permutation(species));
            optimization::count_bonds(bonds, pairs, species);
            objective = optimization::compute_objective(sro, bonds, prefactors, pair_weights,
                                                        target_objective, num_shells, num_species);

          } else if constexpr (SMode == SUBLATTICE_MODE_SPLIT) {
            std::vector<T> objectives(num_sublattices);
            for (auto sigma = 0; sigma < num_sublattices; ++sigma) {
              shuffler.at(sigma).template shuffle<IMode>(species.at(sigma));
              optimization::count_bonds(bonds.at(sigma), pairs.at(sigma), species.at(sigma));
              objective.at(sigma) = optimization::compute_objective(
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
          if (objective_value <= this->search_objective()) {
            // pull in changes from other ranks. Has another rank found a better
            sqs_result<T, SMode> current(objective_value, objective, species, sro);
            spdlog::debug(std::format(
                "[Rank {}, Thread {}] found result with objective {} at iteration {}", this->rank(),
                this->thread_id(), objective_value, rank_t(rstart + i - start).str()));
            this->insert_result(std::move(current));
            // we update the search object. A new entry has been found (on the head rank we will
            // automatically update it)
            this->update_objectives({objective_value, this->nth_best_objective(keep)});

            statistics.log_result(iterations_t{rstart + i - start}, objective_value);
          }
          if constexpr (SMode == SUBLATTICE_MODE_INTERACT)
            shuffler.template shuffle<IMode>(species);
        }
        statistics.tock(tick_loop);

        statistics.add_working(-iterations);
        statistics.add_finished(iterations);

        if (callback.has_value()) {
          spdlog::trace(
              std::format("[Rank {}, Thread {}] firing callback", this->rank(), this->thread_id()));
          callback.value()(sqs_callback_context<T>{stop_source, statistics.data()});
        }
        spdlog::debug(std::format("[Rank {}, Thread {}] finished chunk start={}, end={}",
                                  this->rank(), this->thread_id(), rstart.str(), rend.str()));
        statistics.tock(tick_total);
      };

      spdlog::debug(
          std::format("[Rank {}] spawning thread pool with {} threads (cores available {})",
                      this->rank(), this->num_threads(), std::thread::hardware_concurrency()));
      BS::thread_pool pool(this->num_threads());

      const auto schedule_main_loop = [&] {
        iterations_t chunk_size = this->config.chunk_size;
        pool.detach_blocks(start, end, worker,
                           static_cast<std::size_t>((end - start) / chunk_size));
        pool.wait();
      };
      schedule_main_loop();

      // again we immdeiatly remove the signal handler after they have been used
      if (!mpi_mode) signal::teardown_signal_handlers();

      this->barrier();
#ifdef WITH_MPI
      if (mpi_mode) {
        auto num_results{static_cast<int>(this->results.num_results())};
        auto num_ranks = static_cast<std::size_t>(this->num_ranks());
        if (head) {
          core::tick<TIMING_COMM> tick_comm;
          std::vector remaining(num_ranks, -1);
          this->comm.gather(io::mpi::RANK_HEAD, num_results, remaining.data());

          for (auto i : core::helpers::range(num_ranks))
            if (i > 0)
              spdlog::info(std::format("Expecting {} results from rank {}", remaining[i], i));
          remaining[io::mpi::RANK_HEAD] = 0;
          auto buffer = this->make_empty_result();
          while (sum(remaining) > 0)
            io::mpi::recv_all<sqs_result<T, SMode>>(
                this->comm, std::forward<decltype(buffer)>(buffer),
                [&](auto&& result, auto&& rank) {
                  --remaining[rank];
                  spdlog::trace(
                      "[Rank {}, HEAD] received result from rank {} (objective={}, remaining {}) ",
                      io::mpi::RANK_HEAD, rank, result.objective, remaining[rank]);
                  if (result.objective <= this->search_objective()) {
                    this->insert_result(std::move(result));
                    this->update_objectives({result.objective, this->nth_best_objective(keep)});
                  }
                });

          std::vector<sqs_statistics_data<T>> rank_statistics{statistics.data()};
          rank_statistics.reserve(num_ranks);
          while (rank_statistics.size() < num_ranks)
            io::mpi::recv_all(this->comm, sqs_statistics_data<T>{}, [&](auto&& data, auto) {
              rank_statistics.push_back(std::move(data));
            });
          // print average statistics over all ranks
          sqs_statistics_data<T> average = core::helpers::fold_left(
              rank_statistics, sqs_statistics_data<T>{}, [](auto&& avg, auto&& rank_stats) {
                auto stats = core::sqs_statistics<T>{avg};
                stats.merge(std::forward<sqs_statistics_data<T>>(rank_stats));
                return stats.data();
              });

          sqsgen::detail::log_statistics(std::forward<sqs_statistics_data<T>>(average),
                                         io::mpi::RANK_HEAD, "(averaged)");

          statistics.tock(tick_comm);
        } else {
          core::tick<TIMING_COMM> tick_comm;
          auto results_sent{0};

          auto sqs_results = this->results.results();
          this->comm.gather(io::mpi::RANK_HEAD, num_results);
          for (auto [_, results] : sqs_results)
            for (auto&& sqs_result : results) {
              io::mpi::send(this->comm, std::move(sqs_result), io::mpi::RANK_HEAD);
              spdlog::trace(std::format("[Rank {}] sent result {} / {} to head", this->rank(),
                                        ++results_sent, num_results));
            }
          statistics.tock(tick_comm);
          io::mpi::send(this->comm, statistics.data(), io::mpi::RANK_HEAD);
        }
      }
#endif

      this->barrier();

      // collect statistics data
      auto filtered_results = this->results.remove_duplicates();

      if (!filtered_results.empty()) {
        spdlog::info(std::format("[Rank {}] best_objective={}", this->rank(),
                                 std::get<0>(filtered_results.front())));
        spdlog::info(std::format("[Rank {}] num_best_solutions={}", this->rank(),
                                 std::get<1>(filtered_results.front()).size()));
        spdlog::info(
            std::format("[Rank {}] num_objectives={}", this->rank(), filtered_results.size()));
        spdlog::info(std::format("[Rank {}] num_solutions={}", this->rank(),
                                 sum(filtered_results | views::transform([](auto&& r) {
                                       return std::get<1>(r).size();
                                     }))));
      }

      sqsgen::detail::log_statistics(statistics.data(), this->rank());

      // compute the rank of each permutation, and reorder in case of interact mode
      if (this->is_head())
        for (auto& [_, results] : filtered_results)
          for (auto& result : results)  // use reference here, otherwise
                                        // the reference gets moved
            sqsgen::detail::postprocess_results(result, this->opt_configs);

      std::conditional_t<SMode == SUBLATTICE_MODE_INTERACT, core::optimization_config_data<T>,
                         std::vector<core::optimization_config_data<T>>>
          optimization_configs;
      if constexpr (SMode == SUBLATTICE_MODE_INTERACT)
        optimization_configs = this->opt_configs.front().data();
      else if constexpr (SMode == SUBLATTICE_MODE_SPLIT)
        optimization_configs = as<std::vector>{}(
            this->opt_configs | views::transform([](auto&& c) { return c.data(); }));

      return core::sqs_result_pack<T, SMode>{
          std::move(this->config),
          std::move(optimization_configs),
          std::move(filtered_results),
          statistics.data(),
      };
    }
  };

  namespace detail {

    template <class, class> struct cat_variants {};
    template <class... Args, class... OtherArgs>
    struct cat_variants<std::variant<Args...>, std::variant<OtherArgs...>> {
      using type = std::variant<Args..., OtherArgs...>;
    };

    template <class T, SublatticeMode SMode> using output_t = core::sqs_result_pack<T, SMode>;

    template <class T> using output_for_prec_t
        = std::variant<output_t<T, SUBLATTICE_MODE_INTERACT>, output_t<T, SUBLATTICE_MODE_SPLIT>>;

    using optimizer_output_t
        = cat_variants<output_for_prec_t<float>, output_for_prec_t<double>>::type;

    template <class T>
    optimizer_output_t run_optimization(core::configuration<T>&& conf,
                                        spdlog::level::level_enum log_level = spdlog::level::warn,
                                        std::optional<sqs_callback_t> callback = std::nullopt) {
      if (conf.iteration_mode == ITERATION_MODE_RANDOM
          && conf.sublattice_mode == SUBLATTICE_MODE_INTERACT)
        return optimizer<T, ITERATION_MODE_RANDOM, SUBLATTICE_MODE_INTERACT>(
                   std::forward<core::configuration<T>>(conf))
            .run(log_level, callback);

      else if (conf.iteration_mode == ITERATION_MODE_RANDOM
               && conf.sublattice_mode == SUBLATTICE_MODE_SPLIT)
        return optimizer<T, ITERATION_MODE_RANDOM, SUBLATTICE_MODE_SPLIT>(
                   std::forward<core::configuration<T>>(conf))
            .run(log_level, callback);
      else if (conf.iteration_mode == ITERATION_MODE_SYSTEMATIC
               && conf.sublattice_mode == SUBLATTICE_MODE_INTERACT)
        return optimizer<T, ITERATION_MODE_SYSTEMATIC, SUBLATTICE_MODE_INTERACT>(
                   std::forward<core::configuration<T>>(conf))
            .run(log_level, callback);
      else
        throw std::runtime_error("Invalid configuration of iteration and sublattice mode");
    }

  }  // namespace detail

  inline detail::optimizer_output_t run_optimization(
      std::variant<core::configuration<float>, core::configuration<double>>&& conf,
      spdlog::level::level_enum level = spdlog::level::warn,
      std::optional<sqs_callback_t> callback = std::nullopt) {
    return std::visit(
        [&]<class T>(core::configuration<T>&& c) {
          return detail::optimizer_output_t{detail::run_optimization<T>(
              std::forward<core::configuration<T>>(c), level, callback)};
        },
        std::forward<decltype(conf)>(conf));
  }
}  // namespace sqsgen

#endif  // SQSGEN_SQS_H
