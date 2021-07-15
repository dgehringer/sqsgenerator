//
// Created by dominik on 30.06.21.
//

#ifndef SQSGENERATOR_SQS_HPP
#define SQSGENERATOR_SQS_HPP


#include "types.hpp"
#include "rank.hpp"
#include "utils.hpp"
#include "result.hpp"
#include "settings.hpp"
#include <map>
#include <omp.h>
#include <limits>
#include <vector>
#include <sstream>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <boost/circular_buffer.hpp>



using namespace sqsgenerator::utils;
using namespace sqsgenerator::utils::atomistics;


namespace sqsgenerator {

    typedef size_t AtomicBond[4];

    void count_pairs(const configuration_t &configuration, const std::vector<AtomPair> &pair_list, array_3d_ref_t &bonds, bool clear = false){
        // the array "bonds" must have the following dimensions (nshells, nspecies, nspecies)
        // configuration is already the remapped configuration {H, H, He, He, Ne, Ne} -> {0, 0, 1, 1, 2, 2}
        species_t si, sj;
        // clear the bond count array -> set it to zero
        if (clear) std::fill(bonds.data(), bonds.data() + bonds.num_elements(), 0.0);
        for (const AtomPair &pair : pair_list) {
            auto [i, j, _, shell_index] = pair;
            si = configuration[i];
            sj = configuration[j];
            bonds[shell_index][si][sj]++;
            if (si != sj) bonds[shell_index][sj][si]++;
        }
    }

    void count_pairs_vector(const configuration_t &configuration, const std::vector<size_t> &pair_list, std::vector<double> &bonds, const std::vector<int> &reindexer, size_t nspecies, bool clear) {
        species_t si, sj;
        size_t row_size{3}, npars_reduced {(nspecies * (nspecies -1))/2 + nspecies}, offset;
        int flat_index;
        if (clear) std::fill(bonds.begin(), bonds.end(), 0.0);
        for( auto it=pair_list.begin(); it != pair_list.end(); it+=row_size) {
            si = configuration[*it];
            sj = configuration[*(it+1)];
            if (sj > si) std::swap(si, sj);
            offset = sj * nspecies + si;
            assert(offset < reindexer.size());
            flat_index = reindexer[offset];
            assert(flat_index >= 0 && flat_index < static_cast<int>(reindexer.size()));
            bonds[*(it+2)*npars_reduced + flat_index]++;
        }
    }

    inline
    double calculate_pair_objective_vector(parameter_storage_t &bonds, const parameter_storage_t &prefactors, const parameter_storage_t &parameter_weights, const parameter_storage_t &target_objectives) {
        double total_objective {0.0};
        size_t nparams {bonds.size()};
        for (size_t i = 0; i < nparams; i++) {
            bonds[i] = parameter_weights[i]*(1.0 - bonds[i]*prefactors[i]);
            total_objective += std::abs(bonds[i] - target_objectives[i]);
        }
        return total_objective;
    }


    double calculate_pair_objective(array_3d_ref_t &bonds, const const_array_3d_ref_t &prefactors, const const_array_2d_ref_t &pair_weights, const const_array_3d_ref_t &target_objectives, const std::vector<double> &shell_weights, index_t nspecies){
        double total_objective {0.0}, pair_objective, pair_sro, pair_weight;
        auto nshells {static_cast<index_t>(shell_weights.size())};
        for (index_t i = 0; i < nshells; i++) {
            double shell_weight {shell_weights[i]};
            for (index_t j = 0; j < nspecies; j++) {
                for (index_t k = j; k < nspecies; k++) {
                    pair_weight = pair_weights[j][k];
                    pair_objective = target_objectives[i][j][k];
                    pair_sro =  shell_weight*pair_weight*(1.0 - bonds[i][j][k]*prefactors[i][j][k]);
                    bonds[i][j][k] = pair_sro;
                    bonds[i][k][j] = pair_sro;
                    // we minimize all parameters and get the distance
                    total_objective += std::abs(pair_sro - pair_objective);
                }
            }
        }

        return total_objective;
    }


    std::vector<std::tuple<rank_t, rank_t>> compute_ranks(const IterationSettings &settings, int nthreads) {
        rank_t start_it, end_it, total;
        auto niterations {settings.num_iterations()};
        std::vector<std::tuple<rank_t, rank_t>> ranks(nthreads);
        total = (settings.mode() == random) ? niterations : utils::total_permutations(settings.packed_configuraton());
        for (int thread_id = 0; thread_id < nthreads; thread_id++) {
            start_it = total / nthreads * thread_id;
            end_it = start_it + total / nthreads;
            // permutation sequence indexing starts with one
            if (settings.mode() == systematic) {start_it++; end_it++;}
            end_it = (thread_id == nthreads - 1) ? total : end_it;
            ranks[thread_id] = std::make_tuple(start_it, end_it);
        }
        return ranks;
    }

    std::vector<size_t> convert_pair_list(const std::vector<AtomPair> &pair_list ){
        std::vector<size_t> result;
        for (const auto &pair : pair_list){
            auto [i, j, _, shell_index] = pair;
            result.push_back(i);
            result.push_back(j);
            result.push_back(shell_index);
        }
        return result;
    }
    template<typename T>
    void print_conf(std::vector<T> conf, bool endl = true) {
        std::cout << "{";
        for (size_t i = 0; i < conf.size()-1; ++i) {
            std::cout << conf[i] << ", ";
        }
        std::cout << conf.back() << "}";
        if (endl) std::cout << std::endl;
    }

    std::vector<int> make_reduction_vector(const IterationSettings &settings) {
        int nspecies {static_cast<int>(settings.num_species())};
        std::vector<int> triu;
        for (int i = 0; i < nspecies; i++) { for (int j = i; j < nspecies; j++) triu.push_back(i*nspecies+j); }
        std::vector<int> indices(*std::max_element(triu.begin(), triu.end()) +1);
        std::fill(indices.begin(), indices.end(), -1);
        for (int i = 0; i < static_cast<int>(triu.size()); i++) indices[triu[i]] = i;
        return indices;
    }

    std::tuple<size_t, parameter_storage_t, parameter_storage_t, parameter_storage_t> reduce_weights_matrices(const IterationSettings &settings, const std::vector<int> &reindexer) {
        // We make use of the symmetry property of the weights
        size_t nspecies {settings.num_shells()},
            nshells{settings.num_shells()},
            npars_per_shell_asym {(nspecies * (nspecies -1))/2 + nspecies}, // upper half of a symmetric matrix plus the main diagonal
            reduced_size {nshells * npars_per_shell_asym},
            full_size {nshells * nspecies * nspecies};
        auto [_ , sorted_shell_weights] = settings.shell_indices_and_weights();
        assert(sorted_shell_weights.size() == nshells);
        auto target_objectives_full = settings.target_objective();
        auto prefactors_full = settings.parameter_prefactors();
        auto parameter_weights_full = settings.parameter_weights();
        parameter_storage_t prefactors(reduced_size), parameter_weights(reduced_size), target_objectives(reduced_size);
        size_t offset {0};
        for (size_t shell = 0; shell < nshells; shell++) {
            auto shell_weight {sorted_shell_weights[shell]};
            for (size_t si = 0; si < nspecies; si++) {
                for (size_t sj = si; sj < nspecies; sj++) {
                    int flat_index {reindexer[si*nspecies+sj]};
                    offset = shell*npars_per_shell_asym+flat_index;
                    assert(flat_index >= 0 && flat_index < static_cast<int>(npars_per_shell_asym));
                    prefactors[offset] = prefactors_full[shell][si][sj];
                    target_objectives[offset] = target_objectives_full[shell][si][sj];
                    parameter_weights[offset] = shell_weight*parameter_weights_full[si][sj];
                }
            }
        }
        return std::make_tuple(npars_per_shell_asym, prefactors, parameter_weights, target_objectives);
    }

    static void do_iterations_vector(const IterationSettings &settings) {
        typedef boost::circular_buffer<SQSResult> result_buffer_t;
        std::vector<std::tuple<rank_t, rank_t>> iteration_ranks;
        parameter_storage_t prefactors, parameter_weights, target_objectives;
        result_buffer_t results(settings.num_output_configurations());
        std::vector<size_t> pair_list (convert_pair_list(settings.pair_list()));
        std::vector<size_t> hist(utils::configuration_histogram(settings.packed_configuraton()));
        rank_t nperms = utils::total_permutations(settings.packed_configuraton());
        size_t nshells{settings.num_shells()}, nspecies {settings.num_shells()}, nparams {nshells*nspecies*nspecies}, reduced_size;
        auto reindexer (make_reduction_vector(settings));
        std::tie(reduced_size, prefactors, parameter_weights, target_objectives) = reduce_weights_matrices(settings, reindexer);


        std::cout << "do_iterations_vector: " << nshells << " shells are actually used" << std::endl;




        #pragma omp parallel default(none) shared(std::cout, boost::extents, settings, results, iteration_ranks, hist, nperms, pair_list, prefactors, parameter_weights, target_objectives, reindexer) firstprivate(nspecies, nshells, reduced_size)
        {
            double objective_local;
            uint64_t random_seed_local;
            get_next_configuration_t get_next_configuration;
            int thread_id {omp_get_thread_num()}, nthreads {omp_get_num_threads()};
            #pragma omp single
            iteration_ranks = compute_ranks(settings, nthreads);
            #pragma omp barrier
            auto [start_it, end_it] = iteration_ranks[thread_id];


            configuration_t configuration_local(settings.packed_configuraton());
            parameter_storage_t parameters_local(reduced_size*nshells);

            switch (settings.mode()) {
                case iteration_mode::random:
                {
                    #pragma omp critical
                    {
                        // the random generators only need initialization in case we are in parallel mode
                        // The call to srand is not guaranteed to be thread-safe and may cause a data-race
                        // Each thread keeps it's own state of the whyhash random number generator
                        std::srand(std::time(nullptr)*thread_id);
                        random_seed_local = std::rand()*thread_id;
                    }
                    get_next_configuration = [&random_seed_local](configuration_t &c) {
                        shuffle_configuration(c, &random_seed_local);
                        return true;
                    };
                } break;
                case iteration_mode::systematic: {
                    get_next_configuration = next_permutation;
                    unrank_permutation(configuration_local, hist, nperms, start_it);
                } break;
            }

            for (rank_t i = start_it; i < end_it; i++) {
                get_next_configuration(configuration_local);
                count_pairs_vector(configuration_local, pair_list, parameters_local, reindexer, nspecies, true);
                objective_local = calculate_pair_objective_vector(parameters_local, prefactors, parameter_weights, target_objectives);
                std::cout << i << " | " << objective_local << std::endl;
            }
        }
    }

    static std::vector<SQSResult> do_iterations(const IterationSettings &settings) {
        typedef boost::circular_buffer<SQSResult> result_buffer_t;
        double best_objective {std::numeric_limits<double>::max()};
        auto niterations {settings.num_iterations()};
        auto nspecies {settings.num_species()};
        auto nshells {settings.num_shells()};
        auto nparams {nspecies * nshells * nshells};
        auto parameter_weights(settings.parameter_weights());
        auto parameter_prefactors(settings.parameter_prefactors());
        auto target_objective(settings.target_objective());
        auto pair_list(settings.pair_list());
        result_buffer_t results(settings.num_output_configurations());

        ///////////////////////////////////////////////////////////////////////



        #pragma omp parallel default(none) shared(std::cout, boost::extents, settings, best_objective, results) firstprivate(nparams, nshells, nspecies, niterations, parameter_prefactors, parameter_weights, target_objective)
        {
            int thread_id {omp_get_thread_num()}, nthreads {omp_get_num_threads()};
            rank_t start_it, end_it;
            uint64_t random_seed_local;
            get_next_configuration_t get_next_configuration;
            auto nspecies_idx {static_cast<index_t>(nspecies)};

            // After creating a team of threads, unpack all the informations from the IterationSettings object
            std::vector<shell_t> shells;
            std::vector<double> sorted_shell_weights;
            double objective_local {best_objective};
            double best_objective_local {best_objective};

            array_3d_t parameters_local_storage(boost::extents[static_cast<index_t>(nshells)][nspecies_idx][nspecies_idx]);
            array_3d_ref_t parameters_local(parameters_local_storage);
            configuration_t configuration_local(settings.packed_configuraton());
            const std::vector<AtomPair> &pair_list {settings.pair_list()};

            switch (settings.mode()) {
                case iteration_mode::random:
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
                case iteration_mode::systematic: {
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
            std::tie(shells,sorted_shell_weights) = settings.shell_indices_and_weights();

            int nit {0};
            for (rank_t i = start_it; i < end_it; i++) {
                nit++;
                get_next_configuration(configuration_local);
                count_pairs(configuration_local, pair_list, parameters_local, true);
                objective_local = calculate_pair_objective(parameters_local, parameter_prefactors, parameter_weights, target_objective, sorted_shell_weights, nspecies_idx);
                /*#pragma omp critical
                {
                    std::cout << "[THREAD: " << thread_id << "] loop = " << i << ", objective = " << objective_local << ", num_pairs = " << pair_list.size()
                              << std::endl;
                }*/
                if (objective_local < best_objective_local) {
                    // we do only an atomic read from the global shared variable in case the local one is already satisfied
                    #pragma omp atomic read
                    best_objective_local = best_objective;
                    if (objective_local < best_objective_local) { // We are sure we have found the a new configuration
                        // if we are in random mode we have to check if the map contains the key already
                        SQSResult result(objective_local, {-1}, configuration_local, parameter_storage_t(parameters_local.data(), parameters_local.data() + nparams));
                        #pragma omp critical
                        {
                            results.push_back(result);
                            std::cout << "[THREAD: " << thread_id << "] Found new best_objective = " << best_objective  << ", best_objetive_local = " << best_objective_local << ", current_objective = " << objective_local << ", num_pairs = " << pair_list.size() << std::endl;
                        }
                        // synchronize
                        #pragma omp atomic write
                        best_objective = objective_local;
                        best_objective_local = objective_local;

                    }
                }
            } // for
            #pragma omp critical
            {
                        std::cout << "[THREAD: " << thread_id << "] finished = " << end_it - start_it << " loops, check(" << nit << ")" << std::endl;
            }
        } // #pragma omp parallel
        std::unordered_set<rank_t> ranks;
        std::vector<SQSResult> final_results;
        for (auto &r : results) {
            rank_t rank = rank_permutation(r.configuration(), settings.num_species());
            r.set_rank(rank);
            r.set_configuration(settings.unpack_configuration(r.configuration()));

            /*std::cout << results.size() << " " << r.objective() << " " << r.rank() << " { ";
            for (const auto &sp: r.configuration()) std::cout << static_cast<int>(sp) << " ";
            std::cout << "}" << std::endl;*/

            if (settings.mode() == random && !ranks.insert(rank).second) continue;
            else  final_results.push_back(std::move(r));
        }
        std::cout << "I have found: " << results.size() << " structures, after checking there are " << final_results.size() << " configurations left" << std::endl;
        return final_results;
    }
}

#endif //SQSGENERATOR_SQS_HPP
