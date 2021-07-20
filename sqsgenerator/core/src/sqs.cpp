//
// Created by dominik on 14.07.21.
//

#include "sqs.hpp"
#include <boost/log/trivial.hpp>

namespace sqsgenerator {

    void log_settings(const IterationSettings &settings) {
        std::string mode = ((settings.mode() == random) ? "random" : "systematic");
        BOOST_LOG_TRIVIAL(debug) << "iteration_settings::mode = "  + mode;
        BOOST_LOG_TRIVIAL(debug) << "iteration_settings::num_atoms = " << settings.num_atoms();
        BOOST_LOG_TRIVIAL(debug) << "iteration_settings::num_species = " << settings.num_species();
        BOOST_LOG_TRIVIAL(debug) << "iteration_settings::num_shells = " << settings.num_shells();
        auto [shells, weights] = settings.shell_indices_and_weights();
        BOOST_LOG_TRIVIAL(debug) << "iteration_settings::shell_weights = " + format_dict(shells, weights);
        BOOST_LOG_TRIVIAL(debug) << "iteration_settings::num_iterations = " << settings.num_iterations();
        BOOST_LOG_TRIVIAL(debug) << "iteration_settings::num_output_configurations = " << settings.num_output_configurations();
        BOOST_LOG_TRIVIAL(debug) << "iteration_settings::num_pairs = " << settings.pair_list().size();
        BOOST_LOG_TRIVIAL(debug) << "iteration_settings::occupied_pairs_percentage = " << static_cast<double>(settings.pair_list().size())/static_cast<double>(settings.num_atoms()*(settings.num_atoms()-1)/2)*100.0 << " %";

    }

    void count_pairs(const configuration_t &configuration, const std::vector<size_t> &pair_list,
                            std::vector<double> &bonds, const std::vector<int> &reindexer, size_t nspecies,
                            bool clear) {
        species_t si, sj;
        std::vector<size_t>::difference_type row_size{3};
        size_t npars_reduced{(nspecies * (nspecies - 1)) / 2 + nspecies}, offset;
        int flat_index;
        if (clear) std::fill(bonds.begin(), bonds.end(), 0.0);
        for (auto it = pair_list.begin(); it != pair_list.end(); it += row_size) {
            si = configuration[*it];
            sj = configuration[*(it + 1)];
            if (sj > si) std::swap(si, sj);
            offset = sj * nspecies + si;
            assert(offset < reindexer.size());
            flat_index = reindexer[offset];
            assert(flat_index >= 0 && flat_index < static_cast<int>(reindexer.size()));
            bonds[*(it + 2) * npars_reduced + flat_index]++;
        }
    }

    inline
    double calculate_pair_objective(parameter_storage_t &bonds, const parameter_storage_t &prefactors,
                                           const parameter_storage_t &parameter_weights,
                                           const parameter_storage_t &target_objectives) {
        double total_objective{0.0};
        size_t nparams{bonds.size()};

        for (size_t i = 0; i < nparams; i++) {
            bonds[i] = parameter_weights[i] * (1.0 - bonds[i] * prefactors[i]);
            total_objective += std::abs(bonds[i] - target_objectives[i]);
        }
        return total_objective;
    }

    std::vector<std::tuple<rank_t, rank_t>> compute_ranks(const IterationSettings &settings, int nthreads) {
        rank_t start_it, end_it, total;
        auto niterations{settings.num_iterations()};
        std::vector<std::tuple<rank_t, rank_t>> ranks(nthreads);
        total = (settings.mode() == random) ? niterations : utils::total_permutations(settings.packed_configuraton());
        for (int thread_id = 0; thread_id < nthreads; thread_id++) {
            start_it = total / nthreads * thread_id;
            end_it = start_it + total / nthreads;
            // permutation sequence indexing starts with one
            if (settings.mode() == systematic) {
                start_it++;
                end_it++;
            }
            end_it = (thread_id == nthreads - 1) ? total : end_it;
            ranks[thread_id] = std::make_tuple(start_it, end_it);
        }
        return ranks;
    }

    std::vector<size_t> convert_pair_list(const std::vector<AtomPair> &pair_list) {
        std::vector<size_t> result;
        for (const auto &pair : pair_list) {
            auto[i, j, _, shell_index] = pair;
            result.push_back(i);
            result.push_back(j);
            result.push_back(shell_index);
        }
        return result;
    }

    /**
     * computes a index vector which maps each index of a (symmetric) matrix stored in a flat array into a the offset
     * for a another flat arrray. Due to the symmertric property (\f$i \leq j\f$) we do not have to store all values.
     *
     * Let's consider a \f$N \times N\f$ matrix (E. g. 2 times 2). When storing it as an flat array the index can be
     * computed by \f$i\cdot N  +j \f$. . In case of the \f$2 \times 2\f$ array
     *
     * @param settings iteration settings
     * @return a vector storing the indices of for storing a symmetric matrix
     */
    std::vector<int> make_reduction_vector(const IterationSettings &settings) {
        std::vector<int> triu;
        int nspecies{static_cast<int>(settings.num_species())};
        for (int i = 0; i < nspecies; i++) { for (int j = i; j < nspecies; j++) triu.push_back(i * nspecies + j); }
        std::vector<int> indices(*std::max_element(triu.begin(), triu.end()) + 1);
        std::fill(indices.begin(), indices.end(), -1);
        for (int i = 0; i < static_cast<int>(triu.size()); i++) indices[triu[i]] = i;
        return indices;
    }

    std::tuple<size_t, parameter_storage_t, parameter_storage_t, parameter_storage_t>
    reduce_weights_matrices(const IterationSettings &settings, const std::vector<int> &reindexer) {
        // We make use of the symmetry property of the weights
        size_t nspecies{settings.num_species()},
                nshells{settings.num_shells()},
                npars_per_shell_asym{
                (nspecies * (nspecies - 1)) / 2 + nspecies}, // upper half of a symmetric matrix plus the main diagonal
        reduced_size{nshells * npars_per_shell_asym};
        auto[_, sorted_shell_weights] = settings.shell_indices_and_weights();
        assert(sorted_shell_weights.size() == nshells);
        auto target_objectives_full = settings.target_objective();
        auto prefactors_full = settings.parameter_prefactors();
        auto parameter_weights_full = settings.parameter_weights();
        parameter_storage_t prefactors(reduced_size), parameter_weights(reduced_size), target_objectives(reduced_size);
        size_t offset{0};
        for (size_t shell = 0; shell < nshells; shell++) {
            auto shell_weight{sorted_shell_weights[shell]};
            for (size_t si = 0; si < nspecies; si++) {
                for (size_t sj = si; sj < nspecies; sj++) {
                    int flat_index{reindexer[si * nspecies + sj]};
                    offset = shell * npars_per_shell_asym + flat_index;
                    assert(flat_index >= 0 && flat_index < static_cast<int>(npars_per_shell_asym));
                    prefactors[offset] = prefactors_full[shell][si][sj];
                    target_objectives[offset] = target_objectives_full[shell][si][sj];
                    parameter_weights[offset] = shell_weight * parameter_weights_full[si][sj];
                }
            }
        }
        return std::make_tuple(npars_per_shell_asym, prefactors, parameter_weights, target_objectives);
    }

    parameter_storage_t expand_matrix(const parameter_storage_t &matrix, const IterationSettings &settings,
                                      const std::vector<int> &reindexer) {
        size_t nspecies{settings.num_species()},
                nshells{settings.num_shells()},
                npars_per_shell_asym{
                (nspecies * (nspecies - 1)) / 2 + nspecies}, // upper half of a symmetric matrix plus the main diagonal
        npars_per_shell_sym{nspecies * nspecies}, // upper half of a symmetric matrix plus the main diagonal
        reduced_size{nshells * npars_per_shell_asym},
                full_size{nshells * nspecies * nspecies},
                offset_compact,
                offset_full;

        parameter_storage_t vec(full_size);
        assert(matrix.size() == reduced_size);
        for (size_t shell = 0; shell < nshells; shell++) {
            offset_full = shell * npars_per_shell_sym;
            offset_compact = shell * npars_per_shell_asym;
            for (size_t si = 0; si < nspecies; si++) {
                for (size_t sj = si; sj < nspecies; sj++) {
                    int flat_index{reindexer[si * nspecies + sj]};
                    vec[offset_full + si * nspecies + sj] = matrix[offset_compact + flat_index];
                    if (si != sj) vec[offset_full + sj * nspecies + si] = matrix[offset_compact + flat_index];
                    assert(flat_index >= 0 && flat_index < static_cast<int>(npars_per_shell_asym));
                }
            }
        }
        return vec;
    }

    std::vector<SQSResult> do_pair_iterations(const IterationSettings &settings) {
        log_settings(settings);
        typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point_t;
        typedef boost::circular_buffer<SQSResult> result_buffer_t;
        double best_objective{std::numeric_limits<double>::max()};
        std::vector<std::tuple<rank_t, rank_t>> iteration_ranks;
        std::vector<long> thread_timings;
        parameter_storage_t prefactors, parameter_weights, target_objectives;
        result_buffer_t results(settings.num_output_configurations());
        std::vector<size_t> pair_list(convert_pair_list(settings.pair_list()));
        std::vector<size_t> hist(utils::configuration_histogram(settings.packed_configuraton()));
        rank_t nperms = utils::total_permutations(settings.packed_configuraton());
        size_t nshells{settings.num_shells()}, nspecies{settings.num_species()}, nparams {nshells * nspecies * nspecies}, reduced_size;
        auto reindexer(make_reduction_vector(settings));
        std::tie(reduced_size, prefactors, parameter_weights, target_objectives) = reduce_weights_matrices(settings,
                                                                                                           reindexer);

        auto[shells, shell_weights] = settings.shell_indices_and_weights();

        #pragma omp parallel default(none) shared(std::cout, boost::extents, settings, results, iteration_ranks, hist, nperms, pair_list, prefactors, parameter_weights, target_objectives, reindexer, best_objective, thread_timings) firstprivate(nspecies, nshells, reduced_size)
        {
            double objective_local;
            uint64_t random_seed_local;
            time_point_t start_time, end_time;
            double best_objective_local{best_objective};
            get_next_configuration_t get_next_configuration;
            int thread_id{omp_get_thread_num()}, nthreads{omp_get_num_threads()};
            #pragma omp single
            {
                iteration_ranks = compute_ranks(settings, nthreads);
                thread_timings.resize(nthreads);
            }
            #pragma omp barrier
            auto[start_it, end_it] = iteration_ranks[thread_id];

            configuration_t configuration_local(settings.packed_configuraton());
            parameter_storage_t parameters_local(reduced_size * nshells);

            switch (settings.mode()) {
                case iteration_mode::random: {
                    #pragma omp critical
                    {
                        // the random generators only need initialization in case we are in parallel mode
                        // The call to srand is not guaranteed to be thread-safe and may cause a data-race
                        // Each thread keeps it's own state of the whyhash random number generator
                        std::srand(std::time(nullptr) * (thread_id + 1));
                        random_seed_local = std::rand() * (thread_id + 1); // Otherwise thread 0 generator
                    }
                    get_next_configuration = [&random_seed_local](configuration_t &c) {
                        shuffle_configuration(c, &random_seed_local);
                        return true;
                    };
                }
                    break;
                case iteration_mode::systematic: {
                    get_next_configuration = next_permutation;
                    unrank_permutation(configuration_local, hist, nperms, start_it);
                }
                    break;
            }
            start_time = std::chrono::high_resolution_clock::now();
            for (rank_t i = start_it; i < end_it; i++) {
                get_next_configuration(configuration_local);
                count_pairs(configuration_local, pair_list, parameters_local, reindexer, nspecies, true);
                objective_local = calculate_pair_objective(parameters_local, prefactors, parameter_weights,
                                                                  target_objectives);
                if (objective_local <= best_objective_local) {
                    // we do only an atomic read from the global shared variable in case the local one is already satisfied
                    #pragma omp atomic read
                    best_objective_local = best_objective;
                    if (objective_local <= best_objective_local) { // We are sure we have found the a new configuration
                        // if we are in random mode we have to check if the map contains the key already
                        SQSResult result(objective_local, {-1}, configuration_local, parameters_local);
                        #pragma omp critical
                        results.push_back(result);
                        // synchronize writing to global best objective
                        #pragma omp atomic write
                        best_objective = objective_local;
                        best_objective_local = objective_local;
                    }
                }
            } // for
            end_time = std::chrono::high_resolution_clock::now();
            thread_timings[thread_id] = std::chrono::duration_cast<std::chrono::microseconds>(
                    end_time - start_time).count();
        } // pragma omp parallel

        long total_time {0};
        rank_t total_iterations = 0;
        for (size_t i = 0; i < thread_timings.size(); i++) {
            auto [start_rank, end_rank] = iteration_ranks[i];
            total_time += thread_timings[i];
            total_iterations += (end_rank - start_rank);
            BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::thread::" << static_cast<int>(i) << "::average_time = " << static_cast<double>(thread_timings[i])/(long)(end_rank - start_rank);
        }
        BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::thread::average_time = " << static_cast<double>(total_time)/(long)(total_iterations);
        BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::thread::average_time = " << static_cast<double>(total_time)/(long)(total_iterations*thread_timings.size());

        std::unordered_set<rank_t> ranks;
        std::vector<SQSResult> final_results;
        for (auto &r : results) {
            rank_t rank = rank_permutation(r.configuration(), settings.num_species());
            r.set_rank(rank);
            r.set_configuration(settings.unpack_configuration(r.configuration()));
            r.set_storage(expand_matrix(r.storage(), settings, reindexer));
            assert(r.storage().size() == nparams);
            if (settings.mode() == random && !ranks.insert(rank).second) continue;
            else final_results.push_back(std::move(r));
        }
        return final_results;
    }
}