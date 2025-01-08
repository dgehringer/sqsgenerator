//
// Created by Dominik Gehringer on 05.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_SQS_H
#define SQSGEN_OPTIMIZATION_SQS_H

#include "sqsgen/config.h"
#include "sqsgen/core/shuffle.h"
#include "sqsgen/core/thread_pool.h"
#include "sqsgen/io/mpi.h"
#include "sqsgen/optimization/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  template <class, SublatticeMode> struct sqs_result {};

  template <class T> struct sqs_result<T, SUBLATTICE_MODE_INTERACT> {
    std::optional<rank_t> rank;
    T objective;
    configuration_t species;
    cube_t<T> parameters;
  };

  template <class T> struct sqs_result<T, SUBLATTICE_MODE_SPLIT> {
    std::optional<std::vector<rank_t>> rank;
    std::vector<T> objective;
    std::vector<configuration_t> species;
    std::vector<cube_t<T>> parameters;
  };

  template <class, IterationMode, SublatticeMode> class optimizer {};

  template <class T> class optimizer<T, ITERATION_MODE_RANDOM, SUBLATTICE_MODE_INTERACT> {
    configuration<T> config;
    std::atomic<iterations_t> _finished;
    std::atomic<iterations_t> _scheduled;
    std::atomic<iterations_t> _working;
    std::atomic<T> _best_objective;

  public:
    explicit optimizer(configuration<T> config)
        : config(config), _best_objective(std::numeric_limits<T>::max()) {}

    void run() {
      using namespace sqsgen::core::helpers;
      auto comm = io::MPICommunicator<T>(config.thread_config);

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

      iterations_t chunk_size = config.chunk_size;
      iterations_t iterations = config.iterations.value();
      auto pool = core::thread_pool<core::Task<iterations_t>>(comm.num_threads());

      std::stop_token st = pool.get_stop_token();
      std::function<void(iterations_t)> worker =
          [this, &pool, &species_packed, &shuffling, &worker, &comm, st, pairs,
           num_shells = weights.size(), num_species = sorted.num_species, total = iterations,
           chunk_size, prefactors, target, pair_weights](iterations_t iterations) {
            _working.fetch_add(iterations);
            configuration_t species(species_packed);
            cube_t<usize_t> bonds(num_shells, num_species, num_species);
            cube_t<T> sro(num_shells, num_species, num_species);
            T objective(0);
            for (iterations_t i = 0; i < iterations; ++i) {
              if (st.stop_requested()) return;
              bonds.setConstant(0);
              shuffling.shuffle(species);
              for (auto& [i, j, s] : pairs) {
                auto si{species[i]};
                auto sj{species[j]};
                ++bonds(s, si, sj);
              }
              // symmetrize bonds for each shell and compute objective function
              sro.setConstant(T(0.0));
              objective = 0.0;
              for (auto s = 0; s < num_shells; ++s) {
                for (auto xi = 0; xi < num_species; ++xi) {
                  for (auto eta = xi; eta < num_species; ++eta) {
                    auto pair_bonds = bonds(s, xi, eta) + bonds(s, eta, xi);
                    T sigma_s_xi_eta = T(1.0) - static_cast<T>(pair_bonds) * prefactors(s, xi, eta);
                    sro(s, xi, eta) = sigma_s_xi_eta;
                    sro(s, eta, xi) = sigma_s_xi_eta;
                    objective
                        += pair_weights(s, xi, eta) * absolute(sigma_s_xi_eta - target(s, xi, eta));
                  }
                }
              }
              if (objective <= _best_objective.load()) {
                // pull in changes from other ranks. Has another rank found a better configuration?
                std::cout << comm.rank() << ", " << _best_objective.load() << std::endl;
                comm.synchronize_objective(_best_objective);
                std::cout << "\t" << comm.rank() << ", " << _best_objective.load() << ", " << objective << std::endl;
                // load again - a different thread may also have found a better configuration
                if (objective < _best_objective.load()) {
                  _best_objective.exchange(objective);
                  // we broadcast it to the other ranks, if our configuration is truly the best
                  comm.broadcast_objective(T(objective));
                }

              }
            }
            _working.fetch_add(-iterations);
            comm.synchronize_objective(_best_objective);
            iterations_t finished = _finished.fetch_add(iterations);
            if (finished >= total)
              pool.stop();
            else {
              iterations_t working = _working.load();
              iterations_t next_block = std::min(total - finished - working, chunk_size);
              if (_scheduled.load() + working < total) {
                pool.enqueue_fn(worker, iterations_t(next_block));
                _scheduled.fetch_add(next_block);
              }
            }
          };
      pool.start();
      for (auto n = 0; n <= std::thread::hardware_concurrency(); ++n)
        pool.enqueue_fn(worker, iterations_t(chunk_size));
      pool.join();
    }

  private:
  };
}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_SQS_H
