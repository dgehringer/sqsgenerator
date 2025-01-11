//
// Created by Dominik Gehringer on 05.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_SQS_H
#define SQSGEN_OPTIMIZATION_SQS_H

#include "sqsgen/config.h"
#include "sqsgen/core/shuffle.h"
#include "sqsgen/core/thread_pool.h"
#include "sqsgen/optimization/collection.h"
#include "sqsgen/optimization/comm.h"
#include "sqsgen/optimization/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  template <class, IterationMode, SublatticeMode> class optimizer {};

  template <class T> class optimizer<T, ITERATION_MODE_RANDOM, SUBLATTICE_MODE_INTERACT> {
    configuration<T> config;
    std::atomic<iterations_t> _finished;
    std::atomic<iterations_t> _scheduled;
    std::atomic<iterations_t> _working;
    std::atomic<T> _best_objective;

    void update_objective(comm::MPICommunicator<T>& comm, T objective) {
      if (objective < _best_objective.load()) {
        _best_objective.exchange(objective);
        // we broadcast it to the other ranks, if our configuration is truly the best
        comm.broadcast_objective(T(objective));
      }
    }

  public:
    explicit optimizer(configuration<T> config)
        : config(config), _best_objective(std::numeric_limits<T>::max()) {}

    void run() {
      using namespace sqsgen::core::helpers;
      auto comm = comm::MPICommunicator<T>(config.thread_config);

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

      auto head = comm.is_head();
      iterations_t chunk_size = config.chunk_size;
      iterations_t iterations = config.iterations.value();
      sqs_result_collection<T, SUBLATTICE_MODE_INTERACT> results{};
      core::thread_pool<core::Task<iterations_t>> pool(comm.num_threads());

      const auto schedule_chunk
          = [this, &pool, chunk_size, total = iterations](auto fn, iterations_t finished) {
              iterations_t working = _working.load();
              iterations_t next_block = std::min(total - finished - working, chunk_size);
              if (_scheduled.load() + working <= total) {
                pool.enqueue_fn(fn, iterations_t(next_block));
                _scheduled.fetch_add(next_block);
              }
            };

      const auto pull_results
          = [&, head]() {
            auto buffer = sqs_result<T, SUBLATTICE_MODE_INTERACT>::empty(weights, sorted);
              if (head)
                for (auto&& r : comm.pull_results(buffer)) results.insert_result(std::move(r));
            };

      std::stop_token st = pool.get_stop_token();
      std::function<void(iterations_t)> worker = [this, &pool, &species_packed, &shuffling, &worker,
                                                  &comm, &schedule_chunk, &results, &pull_results,
                                                  st, pairs, num_shells = weights.size(),
                                                  num_species = sorted.num_species,
                                                  total = iterations, prefactors, target,
                                                  pair_weights, head](iterations_t iterations) {
        _working.fetch_add(iterations);
        configuration_t species(species_packed);
        cube_t<usize_t> bonds(num_shells, num_species, num_species);
        cube_t<T> sro(num_shells, num_species, num_species);
        sqs_result<T, SUBLATTICE_MODE_INTERACT> buffer{
            std::nullopt, std::numeric_limits<T>::infinity(), species, sro};
        helpers::scoped_execution([&] { comm.pull_objective(_best_objective); });
        helpers::scoped_execution([&] { pull_results(); });
        for (iterations_t i = 0; i < iterations; ++i) {
          if (st.stop_requested()) return;
          shuffling.shuffle(species);
          helpers::count_bonds(bonds, pairs, species);
          // symmetrize bonds for each shell and compute objective function
          T objective = helpers::compute_objective(sro, bonds, prefactors, pair_weights, target,
                                                   num_shells, num_species);
          if (objective <= _best_objective.load()) {
            // pull in changes from other ranks. Has another rank found a better
            comm.pull_objective(_best_objective);
            update_objective(comm, objective);
            sqs_result<T, SUBLATTICE_MODE_INTERACT> current{std::nullopt, objective, species, sro};
            if (!head)
              comm.send_result(std::move(current));
            else {
              // this sqs_result_collection::insert is thread safe
              results.insert_result(std::forward<decltype(current)>(current));
              pull_results();
            }
          }
        }
        _working.fetch_add(-iterations);
        iterations_t finished = _finished.fetch_add(iterations);
        if (finished >= total)
          pool.stop();
        else
          schedule_chunk(worker, iterations_t(finished));
      };
      pool.start();
      for_each([&worker, &schedule_chunk](auto) { schedule_chunk(worker, 0); }, comm.num_threads());
      pool.join();
      std::cout << comm.rank() << "{" << head << "}" << " -> " << results.num_results()
                << std::endl;
    }

  private:
  };
}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_SQS_H
