//
// Created by dominik on 30.06.21.
//

#ifndef SQSGENERATOR_SQS_HPP
#define SQSGENERATOR_SQS_HPP


#include "types.hpp"
#include "settings.hpp"
#include "containers.hpp"
#include <map>
#include <omp.h>
#include <limits>
#include <vector>
#include <sstream>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <boost/circular_buffer.hpp>



using namespace sqsgenerator::utils;
using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator {

    template<typename MultiArray>
    void count_pairs(const configuration_t &configuration, const std::vector<AtomPair> &pair_list, MultiArray &bonds, bool clear = false){
        typedef typename MultiArray::index index_t;
        typedef typename MultiArray::element T;
        // the array "bonds" must have the following dimensions (nshells, nspecies, nspecies)
        // configuration is already the remapped configuration {H, H, He, He, Ne, Ne} -> {0, 0, 1, 1, 2, 2}
        T increment {static_cast<T>(1.0)};
        species_t si, sj;
        // clear the bond count array -> set it to zero
        if (clear) std::fill(bonds.data(), bonds.data() + bonds.num_elements(), 0.0);
        for (const AtomPair &pair : pair_list) {
            auto [i, j, _, shell_index] = pair;
            si = configuration[i];
            sj = configuration[j];
            bonds[shell_index][si][sj] += increment;
            if (si != sj) bonds[shell_index][sj][si] += increment;
        }
    }


    template<typename MultiArrayBonds, typename MultiArrayWeights>
    typename MultiArrayBonds::element calculate_pair_objective(MultiArrayBonds &bonds, const std::vector<typename MultiArrayBonds::element> &shell_weights, const MultiArrayWeights &pair_weights, size_t nspecies){
        typedef typename MultiArrayBonds::index index_t;
        typedef typename MultiArrayBonds::element T;
        auto nshells {shell_weights.size()};
        T total_objective {0.0};
        for (index_t i = 0; i < nshells; i++) {
            T shell_weight {shell_weights[i]};
            for (index_t j = 0; j < nspecies; j++) {
                for (index_t k = j; k < nspecies; k++) {
                    T pair_weight {pair_weights[j][k]};
                    T pair_sro {shell_weight * (1.0 - bonds[i][j][k]*pair_weight)};
                    bonds[i][j][k] = pair_sro;
                    bonds[i][k][j] = pair_sro;
                    total_objective += std::abs(pair_sro);
                }
            }
        }
        return total_objective;
    }

    void do_iterations(const IterationSettings &settings) {
        typedef array_3d_ref_t::index index_t;
        double best_objective {std::numeric_limits<double>::max()};
        double target_objective {settings.target_objective()};
        int niterations {settings.num_iterations()};
        auto nspecies{settings.num_species()};
        auto nshells {settings.num_shells()};
        auto nparams {nspecies * nshells * nshells};


        typedef boost::circular_buffer<SQSResult> result_buffer_t;
        result_buffer_t results(settings.num_output_configurations());

        omp_set_num_threads(1);
        #pragma omp parallel default(none) shared(std::cout, boost::extents, settings, best_objective, results) firstprivate(nshells, nspecies, target_objective, niterations, nparams)
        {
            rank_t start_it, end_it;
            int thread_id {omp_get_thread_num()}, nthreads {omp_get_num_threads()};
            uint64_t random_seed_local;
            get_next_configuration_t get_next_configuration;
            // After creating a team of threads, unpack all the informations from the IterationSettings object

            double objective_local {best_objective};
            double best_objective_local {best_objective};
            array_3d_t parameters_local(boost::extents[static_cast<index_t>(nshells)][static_cast<index_t>(nspecies)][static_cast<index_t>(nspecies)]);
            configuration_t configuration_local(settings.packed_configuraton());
            const_array_2d_ref_t parameter_weights(settings.parameter_weights());
            const std::vector<AtomPair> &pair_list {settings.pair_list()};
            pair_shell_weights_t shell_weights(settings.shell_weights());


            switch (settings.mode()) {
                case utils::iteration_mode::random:
                {
                    // the random generators only need initialization in case we are in parallel mode
                    #pragma omp critical
                    {
                        // The call to srand is not guaranteed to be thread-safe and may cause a data-race
                        // Each thread keeps it's own state of the whyhash random number generator
                        std::srand(std::time(nullptr)*thread_id);
                        random_seed_local = std::rand()*thread_id;
                    }

                    get_next_configuration = [&random_seed_local](configuration_t &c) {
                        shuffle_configuration(c, &random_seed_local);
                        return true;
                    };

                    start_it = niterations / nthreads * thread_id;
                    end_it = start_it + niterations / nthreads;
                    end_it = (thread_id == nthreads - 1) ? niterations : end_it;
                } break;

                case utils::iteration_mode::systematic: {

                    get_next_configuration = next_permutation;
                    auto total = utils::total_permutations(configuration_local);
                    auto hist = utils::configuration_histogram(configuration_local);

                    start_it = total / nthreads * thread_id + 1;
                    end_it = start_it + total / nthreads + 1;
                    end_it = (thread_id == nthreads - 1) ? total : end_it;

                    unrank_permutation(configuration_local, hist, total, start_it);
                } break;
            }

            // convert the std::map (shell_weights) to a std::vector<double> in ascending order, sorted by the shell
            std::vector<shell_t> shells;
            std::vector<double> sorted_shell_weights;
            for (const auto &weight: shell_weights) shells.push_back(weight.first);
            std::sort(shells.begin(), shells.end());
            for (const auto &shell: shells) sorted_shell_weights.push_back(shell_weights[shell]);
            /*#pragma omp critical
            {
                std::cout << "[THREAD: " << thread_id << "] total_permutations = " << total_permutations << std::endl;
                std::cout << "[THREAD: " << thread_id << "] num_iterations = " << niterations << std::endl;
                std::cout << "[THREAD: " << thread_id << "] shell_weights = " << print_vector(sorted_shell_weights)
                          << std::endl;
                std::cout << "[THREAD: " << thread_id << "] parameter_weights = " << print_vector(
                        std::vector<double>(parameter_weights.data(),
                                            parameter_weights.data() + parameter_weights.num_elements())) << std::endl;
                std::cout << "[THREAD: " << thread_id << "] configuration = " << print_vector(configuration_local)
                          << std::endl;
            }*/


            for (rank_t i = start_it; i < end_it; i++) {

                get_next_configuration(configuration_local);
                count_pairs(configuration_local, pair_list, parameters_local, true);
                objective_local = std::abs(target_objective - calculate_pair_objective(parameters_local, sorted_shell_weights, parameter_weights, nspecies));
                /*#pragma omp critical
                {
                    std::cout << "[THREAD: " << thread_id << "] loop = " << i << ", objective = " << objective_local << ", num_pairs = " << pair_list.size()
                              << std::endl;
                }*/
                if (objective_local <= best_objective_local) {
                    // we do only an atomic read from the global shared variable in case the local one is already satisfied
                    #pragma omp atomic read
                    best_objective_local = best_objective;
                    if (objective_local <= best_objective_local) { // We are sure we have found the a new configuration
                        // if we are in random mode we have to check if the map contains the key already
                        SQSResult result(objective_local, {-1}, configuration_local, parameter_storage_t(parameters_local.data(), parameters_local.data() + nparams));
                        #pragma omp critical
                        {
                            results.push_back(result);
                        }
                        #pragma omp atomic write
                        best_objective = best_objective_local;
                    }
                }
            } // for

        } // #pragma omp parallel

        std::cout << results.size() << std::endl;

    }
}

#endif //SQSGENERATOR_SQS_HPP
