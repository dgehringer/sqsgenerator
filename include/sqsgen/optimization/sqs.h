//
// Created by Dominik Gehringer on 05.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_SQS_H
#define SQSGEN_OPTIMIZATION_SQS_H

#include "sqsgen/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/shuffle.h"
#include "sqsgen/core/thread_pool.h"
#include "sqsgen/optimization/collection.h"
#include "sqsgen/optimization/comm.h"
#include "sqsgen/optimization/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  template <class T, SublatticeMode Mode> struct optimizer_base {
    configuration<T> config;
    std::atomic<iterations_t> _finished;
    std::atomic<iterations_t> _scheduled;
    std::atomic<iterations_t> _working;
    std::atomic<T> _best_objective;
    sqs_result_collection<T, Mode> _results;
    comm::MPICommunicator<T> _comm;
    core::thread_pool<core::Task<rank_t, rank_t>> _pool;
    std::stop_token _stop_token;
    std::optional<int> _rank;
    std::map<std::string, std::atomic<T>> _timings;

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

    template <class Fn>
    void schedule_chunk(Fn&& fn, rank_t total, rank_t start, iterations_t chunk_size) {
      rank_t end{std::min(start + chunk_size, total)};
      _scheduled.fetch_add(iterations_t{end - start});
      std::cout << std::format("\t RANK {} scheduling range {} to {}\n", rank(), start.to_string(), end.to_string());
      _pool.enqueue_fn<rank_t, rank_t>(fn, std::move(start), std::move(end));
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

    auto make_result_puller(bool head, core::structure<T> const& structure,
                            shell_weights_t<T> const& weights) {
      return [&, head] {
        auto buffer = sqs_result<T, Mode>::empty(weights, structure);
        if (head)
          for (auto&& r : this->_comm.pull_results(buffer))
            this->_results.insert_result(std::move(r));
      };
    }

    template<core::helpers::string_literal Name>
    timer<Name> measure() {
      return timer<Name>{_timings};
    }

    explicit optimizer_base(configuration<T>&& config)
        : config(config),
          _best_objective(std::numeric_limits<T>::max()),
          _finished(0),
          _scheduled(0),
          _results(),
          _comm(config.thread_config),
          _pool(_comm.num_threads()),
          _stop_token(_pool.get_stop_token()) {}
  };

  template <class, IterationMode, SublatticeMode> class optimizer {};

  template <class T> class optimizer<T, ITERATION_MODE_RANDOM, SUBLATTICE_MODE_INTERACT>
      : public optimizer_base<T, SUBLATTICE_MODE_INTERACT> {
  public:
    explicit optimizer(configuration<T> config)
        : optimizer_base<T, SUBLATTICE_MODE_INTERACT>(std::move(config)) {}
    void run() {
      using namespace sqsgen::core::helpers;
      auto config = this->config;
      auto structure = config.structure.structure().apply_composition(config.composition);
      /* sort the structure by the sites sublattice index
       * therefore we obtain contiguous memory regions for each sublattice. We till then determine
       * the shuffling bounds.
       *
       * Assume the given configuration, where the number (0,1) denotes the sublattice of each
       * site and the letter (A,B) the species: Region SL1 |       | v       v [0A, 1B, 0B, 1A,
       * 0A, 1A] -> [0A, 0A, 0B, 1A, 1A, 1B] ^       ^ |       | Region SL2
       */
      auto [sorted, bounds, sort_order]
          = helpers::compute_shuffling_bounds(structure, config.composition);
      auto [species_map, species_rmap] = make_index_mapping<specie_t>(sorted.species);
      auto species_packed
          = as<std::vector>{}(sorted.species | views::transform([&species_map](auto&& s) {
                                return species_map.at(s);
                              }));
      auto radii = config.shell_radii[0];
      auto weights = config.shell_weights[0];
      auto prefactors = config.prefactors[0];
      auto target = config.target_objective[0];
      // scale pair weights by w_s, that saves one FLOP in the main loop
      auto pair_weights
          = helpers::scaled_pair_weights(config.pair_weights[0], weights, sorted.num_species);
      auto [pairs, shells_map, shells_rmap] = sorted.pairs(radii, weights);
      std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
        return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
      });
      core::shuffler shuffling(bounds);

      auto head = this->_comm.is_head();
      iterations_t chunk_size = config.chunk_size;
      auto [start, end] = this->iteration_range();
      std::cout << std::format("RANK {} GOT RANGE: {} TO {} (CHUNK_SIZE={})\n", this->rank(),
                               start.to_string(), end.to_string(), chunk_size);

      const auto pull_results = this->make_result_puller(head, sorted, weights);

      std::function<void(rank_t, rank_t)> worker
          = [this, &species_packed, &shuffling, &worker, &pull_results, pairs,
             num_shells = weights.size(), num_species = sorted.num_species, total = end, prefactors,
             target, pair_weights, chunk_size, head](rank_t&& rstart, rank_t&& rend) {
              iterations_t iterations{rend - rstart};
              this->_working.fetch_add(iterations);
              configuration_t species(species_packed);
              cube_t<usize_t> bonds(num_shells, num_species, num_species);
              cube_t<T> sro(num_shells, num_species, num_species);
              sqs_result<T, SUBLATTICE_MODE_INTERACT> buffer{
                  std::nullopt, std::numeric_limits<T>::infinity(), species, sro};
              auto m = this->template measure<"total">();
              this->_working.fetch_add(iterations);
              helpers::scoped_execution([&] { this->template measure<"comm">(); this->pull_objective(); });
              helpers::scoped_execution([&] { this->template measure<"comm">(); pull_results(); });
              for (auto i = rstart; i < rend; ++i) {
                if (this->stop_requested()) return;
                shuffling.shuffle(species);
                helpers::count_bonds(bonds, pairs, species);
                // symmetrize bonds for each shell and compute objective function
                T objective = helpers::compute_objective(sro, bonds, prefactors, pair_weights,
                                                         target, num_shells, num_species);
                if (objective <= this->_best_objective.load()) {
                  // pull in changes from other ranks. Has another rank found a better
                  auto _ = this->template measure<"comm">();
                  this->pull_objective();
                  this->update_objective(objective);
                  sqs_result<T, SUBLATTICE_MODE_INTERACT> current{std::nullopt, objective, species,
                                                                  sro};
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
              std::cout << std::format("RANK {} finished rank {} to {}  of {}\n", this->rank(),
                                       rstart.to_string(), rend.to_string(), finished);
              if (finished >= total)
                this->stop();
              else
                this->schedule_chunk(worker, total, rend, chunk_size);
            };
      this->start();
      for_each(
          [&, chunk_size](auto i) {
            this->schedule_chunk(worker, end, rank_t{i * chunk_size}, chunk_size);
          },
          this->_comm.num_threads());
      this->join();
      this->pull_objective();
      pull_results();

      for (const auto& [what, time]: this->_timings) {
        std::cout << std::format("RANK {}: Time for {}: {} per iteration {}\n", this->rank(), what, time.load(), time.load() / static_cast<T>(iterations_t(end-start)));
      }
    }

  private:
  };
}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_SQS_H
