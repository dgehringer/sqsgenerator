//
// Created by Dominik Gehringer on 05.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_SQS_H
#define SQSGEN_OPTIMIZATION_SQS_H

#include "sqs.h"
#include "sqsgen/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/shuffle.h"
#include "sqsgen/core/thread_pool.h"
#include "sqsgen/optimization/collection.h"
#include "sqsgen/optimization/comm.h"
#include "sqsgen/optimization/helpers.h"
#include "sqsgen/types.h"

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

  }  // namespace detail

  template <class T, SublatticeMode Mode> struct optimizer_base {
    configuration<T> config;
    std::atomic<iterations_t> _finished;
    std::atomic<iterations_t> _offset;
    std::atomic<iterations_t> _working;
    std::atomic<T> _best_objective;
    sqs_result_collection<T, Mode> _results;
    comm::MPICommunicator<T> _comm;
    core::thread_pool<core::Task<rank_t, rank_t>> _pool;
    std::stop_token _stop_token;
    std::optional<int> _rank;
    std::map<std::string, std::atomic<T>> _timings;
    std::vector<detail::optimization_config<T, Mode>> opt_configs;

    template <core::helpers::string_literal Name> struct timer {
      std::chrono::time_point<std::chrono::high_resolution_clock> _start;
      std::map<std::string, std::atomic<T>>& timings;

      explicit timer(std::map<std::string, std::atomic<T>>& t) : timings(t) {
        if (!timings.contains(Name.data)) timings.emplace(Name.data, T(0));
        _start = std::chrono::high_resolution_clock::now();
      }
      ~timer() {
        timings.at(Name.data).fetch_add(
            static_cast<T>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                               std::chrono::high_resolution_clock::now() - _start)
                               .count()));
      }
    };

    void update_objective(T objective) {
      if (objective < _best_objective.load()) {
        _best_objective.exchange(objective);
        // we broadcast it to the other ranks, if our configuration is truly the best
        _comm.broadcast_objective(T(objective));
      }
    }

    void pull_objective() { _comm.pull_objective(_best_objective); }

    void send_result(sqs_result<T, Mode>&& result) {
      _comm.send_result(std::forward<sqs_result<T, Mode>>(result));
    }

    void stop() { _pool.stop(); }

    bool stop_requested() const { return _stop_token.stop_requested(); }

    void start() { _pool.start(); }

    void join() { _pool.join(); }

    auto make_scheduler(rank_t start, rank_t end, iterations_t chunk_size) {
      return [this, start, end, chunk_size]<class Fn>(Fn&& fn) {
        auto offset = _offset.load(std::memory_order_relaxed);
        rank_t last{std::min(rank_t{start + offset + chunk_size}, end)};
        iterations_t num_iterations{last - (start + offset)};
        if (num_iterations == 0) return;
        _offset.fetch_add(num_iterations);
        _pool.enqueue_fn<rank_t, rank_t>(fn, rank_t{start + offset}, rank_t{last});
      };
    };

    int rank() {
      if (!_rank.has_value()) _rank = _comm.rank();
      return _rank.value();
    }

    bounds_t<rank_t> iteration_range() {
      rank_t iterations{config.iterations.value()};
      auto r = this->rank();
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

    auto make_result_puller(bool head) {
      const auto make_empty = [&config = opt_configs] {
        using namespace sqsgen::core::helpers;
        if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
          return sqs_result<T, Mode>::empty(config.front().shell_weights, config.front().structure);
        } else if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
          return sqs_result<T, Mode>::empty(
              as<std::vector>{}(config
                                | views::transform([](auto&& c) { return c.shell_weights; })),
              as<std::vector>{}(config | views::transform([](auto&& c) { return c.structure; })));
        }
      };
      return [&, head, make_empty] {
        auto buffer = make_empty();
        if (head)
          for (auto&& r : this->_comm.pull_results(buffer))
            this->_results.insert_result(std::move(r));
      };
    }

    template <core::helpers::string_literal Name> timer<Name> measure() {
      return timer<Name>{_timings};
    }

    template <class Fn>
    detail::lift_t<std::decay_t<std::invoke_result_t<Fn, detail::optimization_config<T, Mode>>>,
                   Mode>
    transpose_setting(Fn&& fn) {
      if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
        return fn(opt_configs.front());
      } else if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
        return core::helpers::as<std::vector>(
            opt_configs | std::ranges::views::transform(std::forward<Fn>(fn)));
      }
      throw std::invalid_argument("unrecognized operation");
    }

    explicit optimizer_base(configuration<T>&& config)
        : config(config),
          _best_objective(std::numeric_limits<T>::max()),
          _finished(0),
          _offset(0),
          _results(),
          _comm(config.thread_config),
          _pool(_comm.num_threads()),
          _stop_token(_pool.get_stop_token()),
          opt_configs(detail::optimization_config<T, Mode>::from_config(config)) {}
  };

  template <class T, IterationMode IMode, SublatticeMode SMode> class optimizer
      : public optimizer_base<T, SMode> {
  public:
    explicit optimizer(configuration<T>&& config)
        : optimizer_base<T, SMode>(std::forward<configuration<T>>(config)) {}
    void run() {
      using namespace sqsgen::core::helpers;

      /* sort the structure by the sites sublattice index
       * therefore we obtain contiguous memory regions for each sublattice. We till then determine
       * the shuffling bounds.
       *
       * Assume the given configuration, where the number (0,1) denotes the sublattice of each
       * site and the letter (A,B) the species: Region SL1 |       | v       v [0A, 1B, 0B, 1A,
       * 0A, 1A] -> [0A, 0A, 0B, 1A, 1A, 1B] ^       ^ |       | Region SL2
       */

      auto head = this->_comm.is_head();
      iterations_t chunk_size = this->config.chunk_size;
      auto [start, end] = this->iteration_range();
      std::cout << std::format("RANK {} GOT RANGE: {} TO {} (CHUNK_SIZE={}, NUM_PAIRS=)\n",
                               this->rank(), start.to_string(), end.to_string(), chunk_size);
      iterations_t total{end - start};
      const auto pull_results = this->make_result_puller(head);
      const auto schedule_chunk = this->make_scheduler(start, end, chunk_size);

      auto num_sublattices = this->opt_configs.size();
      auto pairs{this->transpose_setting([](auto&& c) { return c.pairs; })};
      auto prefactors{this->transpose_setting([](auto&& c) { return c.prefactors; })};
      auto pair_weights{this->transpose_setting([](auto&& c) { return c.pair_weights; })};
      auto target_objective{this->transpose_setting([](auto&& c) { return c.target_objective; })};
      auto shuffler{this->transpose_setting([](auto&& c) { return c.shuffler; })};
      auto species{this->transpose_setting([](auto&& c) { return c.species_packed; })};
      auto num_shells{this->transpose_setting([](auto&& c) { return c.shell_weights.size(); })};
      auto num_species{this->transpose_setting([](auto&& c) { return c.sorted.num_species; })};

      std::function<void(rank_t, rank_t)> worker = [&](rank_t&& rstart, rank_t&& rend) {
        iterations_t iterations{rend - rstart};
        this->_working.fetch_add(iterations);

        auto bonds{this->transpose_setting([](auto&& c) {
          return cube_t<usize_t>(c.shell_weights.size(), c.sorted.num_species,
                                 c.sorted.num_species);
        })};
        auto sro{this->transpose_setting([](auto&& c) {
          return cube_t<T>(c.shell_weights.size(), c.sorted.num_species, c.sorted.num_species);
        })};

        if (this->_finished.load(std::memory_order_relaxed) >= total) {
          std::cout << "ALREADY-FINISHED: " << this->_finished.load(std::memory_order_relaxed)
                    << std::endl;
          return;
        }
        auto objective = this->transpose_setting([](auto&& c) { return T(0); });
        auto m = this->template measure<"total">();
        this->_working.fetch_add(iterations);
        helpers::scoped_execution([&] { this->pull_objective(); });
        helpers::scoped_execution([&] { pull_results(); });
        for (auto i = rstart; i < rend; ++i) {
          if (this->stop_requested()) return;
          if constexpr (SMode == SUBLATTICE_MODE_INTERACT) {
            shuffler.shuffle(species);
            helpers::count_bonds(bonds, pairs, species);
            objective = helpers::compute_objective(sro, bonds, prefactors, pair_weights,
                                                   target_objective, num_shells, num_species);
          } else if constexpr (SMode == SUBLATTICE_MODE_SPLIT) {
            std::vector<T> objectives(num_sublattices);
            for (auto sigma = 0; sigma < num_sublattices; ++sigma) {
              shuffler.at(sigma).shuffle(species.at(sigma));
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
            auto _ = this->template measure<"sync">();
            this->pull_objective();
            this->update_objective(objective_value);
            sqs_result<T, SMode> current{std::nullopt, objective, species, sro};
            if (!head)
              this->send_result(std::move(current));
            else {
              // this sqs_result_collection::insert is thread safe
              this->_results.insert_result(std::forward<decltype(current)>(current));
              pull_results();
            }
          }
        }
        this->_working.fetch_add(-iterations);

        iterations_t finished = this->_finished.fetch_add(iterations);
        if (finished + iterations >= (end - start))
          this->stop();
        else
          schedule_chunk(worker);
      };
      this->start();
      for_each([&](auto) { schedule_chunk(worker); }, this->_comm.num_threads());
      this->join();
      this->pull_objective();
      pull_results();

      for (const auto& [what, time] : this->_timings) {
        std::cout << std::format("RANK {}: Time for {}: {} per iteration {}\n", this->rank(), what,
                                 time.load(),
                                 time.load() / static_cast<T>(iterations_t(end - start)));
      }
    }

  private:
  };
}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_SQS_H
