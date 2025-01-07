//
// Created by Dominik Gehringer on 05.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_SQS_H
#define SQSGEN_OPTIMIZATION_SQS_H

#include <core/shuffle.h>

#include "sqsgen/config.h"
#include "sqsgen/core/thread_pool.h"
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
    using iterations_t = unsigned long long;
    configuration<T> config;
    std::atomic<iterations_t> _finished;
    std::atomic<iterations_t> _scheduled;

  public:
    explicit optimizer(configuration<T> config) : config(config) {}

    void run() {
      using namespace sqsgen::core::helpers;
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
      auto [pairs, shells_map, shells_rmap] = sorted.pairs(radii, weights);
      std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
        return absolute(p.i - p.j) < absolute(q.i - q.j) && p.i < q.i;
      });
      core::shuffler shuffling(bounds);

      iterations_t block_size = 100000;
      iterations_t iterations = 100000000 - 10000;
      auto pool
          = core::thread_pool<core::Task<iterations_t, bool>>(std::thread::hardware_concurrency());

      std::stop_token st = pool.get_stop_token();

      std::function<void(iterations_t, bool)> worker
          = [this, &pool, &species_packed, &shuffling, &worker, st, pairs, num_shells = weights.size(),
             num_species = sorted.num_species, total = iterations,
             block_size](iterations_t iterations, bool last) {
              configuration_t species(species_packed);
              cube_t<usize_t> bonds(num_shells, num_species, num_species);
              for (iterations_t i = 0; i < iterations; ++i) {
                if (st.stop_requested()) break;
                bonds.setConstant(0);
                shuffling.shuffle(species);
                for (auto& [i, j, s] : pairs) {
                  auto si{species[i]};
                  auto sj{species[j]};
                  ++bonds(s, si, sj);
                }
              }
              iterations_t finished = _finished.fetch_add(iterations);
              std::cout << std::format("{}/{}", finished, total) << std::endl;
              if (last)
                pool.stop();
              else {
                iterations_t next_block = std::min(total - finished, block_size);
                pool.enqueue_fn(worker, iterations_t(next_block), finished + next_block == total);
                _scheduled.fetch_add(next_block);
              }
            };
      pool.start();
      for (auto n = 0; n <= std::thread::hardware_concurrency(); ++n)
       pool.enqueue_fn(worker, iterations_t(block_size), false);




      pool.join();
    }

  private:
  };
}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_SQS_H
