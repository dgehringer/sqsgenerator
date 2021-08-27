//
// Created by dominik on 14.07.21.
//

#include "sqs.hpp"
#if defined(_OPENMP)
#include <omp.h>
#endif
#if defined(USE_MPI)
#include <mpi.h>
#define TAG_BETTER_OBJECTIVE 1
#define TAG_COLLECT 2
#define HEAD_RANK 0
#endif
#include <chrono>
#include <unordered_set>
#include <boost/circular_buffer.hpp>
#include <boost/log/trivial.hpp>

namespace sqsgenerator {



    std::string format_sqs_result(const sqsgenerator::SQSResult &result) {
        std::stringstream message;
        message << "{rank: " << result.rank() << ", objective: " << result.objective() << ", configuration: " << format_vector<sqsgenerator::species_t, int>(result.configuration()) << "}";
        return message.str();
    }

    void log_settings(const std::string &function_name, const IterationSettings &settings) {
        std::string mode = ((settings.mode() == random) ? "random" : "systematic");
        BOOST_LOG_TRIVIAL(debug) << function_name << "::settings::mode = "  + mode;
        BOOST_LOG_TRIVIAL(debug) << function_name << "::settings::num_atoms = " << settings.num_atoms();
        BOOST_LOG_TRIVIAL(debug) << function_name << "::settings::num_species = " << settings.num_species();
        BOOST_LOG_TRIVIAL(info) << function_name << "::settings::num_shells = " << settings.num_shells();
        auto [shells, weights] = settings.shell_indices_and_weights();
        BOOST_LOG_TRIVIAL(info) << function_name << "::settings::shell_weights = " + format_dict(shells, weights);
        BOOST_LOG_TRIVIAL(info) << function_name << "::settings::num_iterations = " << settings.num_iterations();
        BOOST_LOG_TRIVIAL(debug) << function_name << "::settings::num_output_configurations = " << settings.num_output_configurations();
        BOOST_LOG_TRIVIAL(debug) << function_name << "::settings::num_pairs = " << settings.pair_list().size();
    }

    inline
    void count_pairs(const configuration_t &configuration, const std::vector<size_t> &pair_list,
                     std::vector<double> &bonds,
                     size_t nspecies,
                     bool clear) {
        std::vector<size_t>::difference_type row_size{3};
        size_t num_sro_params {nspecies * nspecies};
        if (clear) std::fill(bonds.begin(), bonds.end(), 0.0);
        for (auto it = pair_list.begin(); it != pair_list.end(); it += row_size) {
            auto si {configuration[*it]};
            auto sj {configuration[*(it + 1)]};
            auto shell{*(it + 2)};
            bonds[shell * num_sro_params + sj * nspecies + si]++;
            // write to (sj,si) only if si != sj
            if (si != sj) bonds[shell * num_sro_params + si * nspecies + sj]++;
        }
    }

    inline
    double calculate_pair_objective(parameter_storage_t &bonds, const parameter_storage_t &prefactors,
                                           const parameter_storage_t &parameter_weights,
                                           const parameter_storage_t &target_objectives) {
        double total_objective {0.0};
        size_t nparams {bonds.size()};
        for (size_t i = 0; i < nparams; i++) {
            bonds[i] =  (1.0 - bonds[i] * prefactors[i]);
            total_objective += parameter_weights[i] * std::abs(bonds[i] - target_objectives[i]);
        }
        return total_objective;
    }

    rank_iteration_map_t compute_ranks(const IterationSettings &settings, const std::vector<int> &threads_per_rank) {
        auto thread_count {0};
        rank_iteration_map_t rank_map;
        auto niterations {settings.num_iterations()};
        auto num_mpi_ranks {static_cast<int>(threads_per_rank.size())};
        auto nthreads {std::accumulate(threads_per_rank.begin(), threads_per_rank.end(), 0)};

        total = (settings.mode() == random) ? niterations : utils::total_permutations(settings.packed_configuraton());
        for (int mpi_rank = 0; mpi_rank < num_mpi_ranks; mpi_rank++) {
            auto threads_in_mpi_rank = threads_per_rank[mpi_rank];
            std::map<int, std::tuple<rank_t, rank_t>> local_rank_map;
            for (int local_thread_id = 0; local_thread_id < threads_in_mpi_rank; local_thread_id++) {
                auto start_it = total / nthreads * thread_count;
                auto end_it = start_it + total / nthreads;
                // permutation sequence indexing starts with one
                if (settings.mode() == systematic) {
                    start_it++;
                    end_it++;
                }
                end_it = (thread_count == nthreads - 1) ? total : end_it;
                local_rank_map.emplace(std::make_pair(local_thread_id, std::make_tuple(start_it, end_it)));
                thread_count++;
            }
            rank_map.emplace(std::make_pair(mpi_rank, local_rank_map));
        }
        assert(thread_count == nthreads);
        return rank_map;
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


    std::tuple<std::vector<SQSResult>, timing_map_t> do_pair_iterations(const IterationSettings &settings) {
        typedef boost::circular_buffer<SQSResult> result_buffer_t; // this typedef is an implementation detail and used just in this method
        auto threads_per_rank {settings.threads_per_rank()};
        int mpi_num_ranks, mpi_rank;

#if defined(USE_MPI)
        int mpi_all_gather_err, mpi_initialized, mpi_thread_level_support_provided, mpi_thread_level_support_required {MPI_THREAD_SERIALIZED};
        MPI_Initialized(&mpi_initialized);
        if (!mpi_initialized) { // Initialize MPI runtime in case we have none yet
            BOOST_LOG_TRIVIAL(info) << "do_pair_iterations:: MPI Runtime was not initialized. I'm going to do that!";
            MPI_Init_thread(nullptr, nullptr, mpi_thread_level_support_required, &mpi_thread_level_support_provided);
        }
        else MPI_Query_thread(&mpi_thread_level_support_provided); // Ensure that we can use the MPI runtime in the multi-threaded-section
        if (mpi_thread_level_support_provided < mpi_thread_level_support_required) throw std::runtime_error("MPI threading level support is not fulfilling the requirements. I need at least 'MPI_THREAD_SERIALIZED'");

        MPI_Comm_size(MPI_COMM_WORLD, &mpi_num_ranks);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

        if (mpi_rank == HEAD_RANK) {
            if (static_cast<int>(settings.threads_per_rank().size()) != mpi_num_ranks) throw std::runtime_error("Number if ranks does not match the number of entries in the thread map");
            BOOST_LOG_TRIVIAL(info) << "do_pair_iterations::mpi::num_ranks = " << mpi_num_ranks;
            log_settings("do_pair_iterations", settings);
        }
#else
        mpi_num_ranks = 1;
        mpi_rank = 0;
        log_settings("do_pair_iterations", settings);
#endif
        // In case a negative number is specified we try to get as many threads as possible
        if( threads_per_rank[mpi_rank] < 0) {
            threads_per_rank[mpi_rank] = omp_get_max_threads();
            BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::rank::" << mpi_rank << "::num_threads_requested::default = " << threads_per_rank[mpi_rank];

        }
        auto num_threads_per_rank = threads_per_rank[mpi_rank];
        BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::rank::" << mpi_rank << "::num_threads_requested = " << num_threads_per_rank;

        timing_map_t thread_timings;
        rank_iteration_map_t iteration_ranks;
        double best_objective{std::numeric_limits<double>::max()};
        rank_t nperms = utils::total_permutations(settings.packed_configuraton());

        result_buffer_t results(settings.num_output_configurations());

        std::vector<size_t> pair_list(convert_pair_list(settings.pair_list()));
        std::vector<size_t> hist(utils::configuration_histogram(settings.packed_configuraton()));

        size_t nshells {settings.num_shells()}
        size_t nspecies {settings.num_species()}
        size_t nparams {nshells * nspecies * nspecies};

        parameter_storage_t targets_objectives {settings.target_objective()};
        parameter_storage_t parameter_weights {settings.parameter_weights()};
        parameter_storage_t parameter_prefactors {settings.parameter_prefactors()};

        omp_set_num_threads(num_threads_per_rank);
        #pragma omp parallel default(shared) firstprivate(nspecies, nshells, nparams, mpi_rank, mpi_num_ranks)
        {
            uint64_t random_seed_local;
            double best_objective_local {best_objective};
            get_next_configuration_t get_next_configuration;
            int thread_id {omp_get_thread_num()}, nthreads {omp_get_num_threads()};
            configuration_t configuration_local(settings.packed_configuraton());
            parameter_storage_t parameters_local(nparams);

            // we have to synchronize the threads before we go on with initialization

            #pragma omp single
            {
                thread_timings.emplace(mpi_rank,  std::vector<double>(nthreads));
                bool need_redistribution_local {nthreads != threads_per_rank[mpi_rank]}, need_redistribution;
#if defined (USE_MPI)
                bool need_redistribution_global;
                bool need_redistribution_other[mpi_num_ranks]; // Check if any of the other MPI ranks got less threads than expected
                MPI_Allgather(&need_redistribution_local, 1, MPI_CXX_BOOL, need_redistribution_other, 1, MPI_CXX_BOOL, MPI_COMM_WORLD);
                for (int i = 0; i < mpi_num_ranks; i++) if (need_redistribution_other[i])  need_redistribution_global = need_redistribution_other[i];
                BOOST_LOG_TRIVIAL(trace) << "do_pair_iterations::rank::" << mpi_rank << "::redistribution_requests = " +
                            format_vector(std::vector<bool>(need_redistribution_other, need_redistribution_other+mpi_num_ranks));

                need_redistribution = need_redistribution_local or need_redistribution_global;
                MPI_Barrier(MPI_COMM_WORLD);
#else
                need_redistribution = need_redistribution_local;
#endif
                if (need_redistribution) { // The work has to be distributed on a different amount of total threads, than what whe expected
                    threads_per_rank[mpi_rank] = nthreads;
#if defined(USE_MPI)
                    int buf_threads_per_rank[mpi_num_ranks]; // Inform the other ranks about the number of threads available at the own rank
                    mpi_all_gather_err = MPI_Allgather(&nthreads, 1, MPI_INT, buf_threads_per_rank, 1, MPI_INT, MPI_COMM_WORLD);
                    if (mpi_all_gather_err != MPI_SUCCESS) {
                        BOOST_LOG_TRIVIAL(error) << "do_pair_iterations::rank::" << mpi_rank << ": MPI_Alltoall failed";
                        throw std::runtime_error("MPI_Alltoall failed on rank #"+std::to_string(mpi_rank));
                    }
                    BOOST_LOG_TRIVIAL(trace) << "do_pair_iterations::rank::" << mpi_rank << "::actual_threads_per_rank = " +
                                                format_vector(std::vector<int>(buf_threads_per_rank, buf_threads_per_rank+mpi_num_ranks));
                    for (int i = 0; i < mpi_num_ranks; i++) threads_per_rank[i] = buf_threads_per_rank[i];
#endif
                }

                iteration_ranks = compute_ranks(settings, threads_per_rank);
            }
            #pragma omp barrier
            auto [start_it, end_it] = iteration_ranks[mpi_rank][thread_id];
            #pragma omp critical
            {
                BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::rank::" << mpi_rank << "::thread::" << thread_id << "::iteration_start = " << start_it;
                BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::rank::" << mpi_rank << "::thread::" << thread_id << "::num_iterations = " << (end_it - start_it);
                BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::rank::" << mpi_rank << "::thread::" << thread_id << "::iteration_end = " << end_it;
            }

            switch (settings.mode()) {
                case iteration_mode::random: {
                    #pragma omp critical
                    {
                        /* the random generators only need initialization in case we are in parallel mode
                         * The call to srand is not guaranteed to be thread-safe and may cause a data-race
                         * Each thread keeps it's own state of the whyhash random number generator
                         */
                        auto current_time {std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()};
                        std::srand(current_time * (thread_id + 1));
                        random_seed_local = std::rand() * (thread_id + 1);
                        BOOST_LOG_TRIVIAL(trace) << "do_pair_iterations::rank::" << mpi_rank << "::thread::" << thread_id << "::random_seed = " << random_seed_local;
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

            time_point_t start_time = std::chrono::high_resolution_clock::now();
            for (rank_t i = start_it; i < end_it; i++) {
                get_next_configuration(configuration_local);
                count_pairs(configuration_local, pair_list, parameters_local, nspecies, true);
                auto objective_local = calculate_pair_objective(parameters_local, parameter_prefactors, parameter_weights,
                                                                  target_objectives);
                if (objective_local <= best_objective_local) {

#if defined(USE_MPI)
                    /*
                     * We check (at one thread at a time) if any of the other ranks, has populated a better objective.
                     * Therefore we check if any messages have, arrived at our own rank. If, so we read all messages
                     * , in case more than one rank has found something, and upate your rank, and thread internal
                     * "best_objective" value.
                     */
                    #pragma omp critical
                    {
                        int have_message;
                        MPI_Request request;
                        MPI_Status request_status;
                        MPI_Iprobe(MPI_ANY_SOURCE, TAG_BETTER_OBJECTIVE, MPI_COMM_WORLD, &have_message, &request_status);
                        while (have_message) {
                            double global_best_objective;
                            int source_rank = request_status.MPI_SOURCE;
                            MPI_Irecv(&global_best_objective, 1, MPI_DOUBLE, source_rank, TAG_BETTER_OBJECTIVE, MPI_COMM_WORLD, &request);
                            MPI_Wait(&request, &request_status);
                            if (global_best_objective < best_objective) {
                                #pragma omp atomic write
                                best_objective = global_best_objective;
                            }
                            // else assert( best_objective >= global_best_objective);
                            MPI_Iprobe(MPI_ANY_SOURCE, TAG_BETTER_OBJECTIVE, MPI_COMM_WORLD, &have_message, &request_status);
                        }
                    }
#endif
                    // we do only an atomic read from the global shared variable in case the current is smaller than the thread internal
                    #pragma omp atomic read
                    best_objective_local = best_objective;

                    SQSResult result(objective_local, {-1}, configuration_local, parameters_local);
                    #pragma omp critical
                    results.push_back(result);
                    // synchronize writing to global best objective, only if the local one is really better
                    if (objective_local < best_objective_local){
                        #pragma omp atomic write
                        best_objective = objective_local;
                        best_objective_local = objective_local;
#if defined(USE_MPI)
                        /*
                         * Notify the other processes that we have found. Just one of the threads is allowed to send the
                         * messages. However we only do that rather expensive operation only in case the objective is
                         * strictly smaller rather than <=
                         */
                        #pragma omp critical
                        {
                            auto handle_count{0};
                            MPI_Request req_notify[mpi_num_ranks - 1];
                            for (int rank = 0; rank < mpi_num_ranks; rank++) {
                                if (rank == mpi_rank) continue;
                                MPI_Isend(&objective_local, 1, MPI_DOUBLE, rank, TAG_BETTER_OBJECTIVE, MPI_COMM_WORLD,
                                          &req_notify[handle_count++]);
                            }
                        }
                        // We do not wait for the request handles to finish
#endif
                    }
                }
            } // for
            time_point_t end_time = std::chrono::high_resolution_clock::now();
            auto avg_loop_time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count()) /(long)(end_it - start_it);
            #pragma omp critical
            {
                BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::rank::" << mpi_rank << "::thread::" << thread_id << "::avg_loop_time = " << avg_loop_time;
                thread_timings[mpi_rank][thread_id] = avg_loop_time;
            };
        } // pragma omp parallel

        // After the main loop has finished, we copy (move) the values from the circular buffer into a vector
        std::vector<SQSResult> tmp_results, final_results;
        for (auto &r : results) tmp_results.push_back(std::move(r));

#if defined (USE_MPI)
        /*
         * If MPI is used we have to send timing information, therefore a few things are needed
         *  - We need to now how many threads were actually allocated on our own MPI rank (actual_threads_on_rank)
         *  - A storage where to store the actual number of threads per MPI rank (num_actual_threads_on_rank)
         *  - A storage where to store the actual number of found results per MPI rank (num_sqs_results)
         */
        int actual_threads_on_rank {static_cast<int>(thread_timings[mpi_rank].size())};
        double buf_timings[actual_threads_on_rank];
        species_t buf_conf[settings.num_atoms()];
        std::vector<int> num_sqs_results, num_actual_threads_on_rank;
        if (mpi_rank == HEAD_RANK)  {
            num_sqs_results.resize(mpi_num_ranks);
            num_actual_threads_on_rank.resize(mpi_num_ranks);
        }

        // Synchornize the actual number of threads per rank (num_actual_threads_on_rank) and the number of SQS results
        // (num_sqs_results)
        int num_results {static_cast<int>(results.size())};
        // Gater the number of final configurations and the number of threads on each rank
        MPI_Gather(&num_results, 1, MPI_INT, num_sqs_results.data(), 1, MPI_INT, HEAD_RANK, MPI_COMM_WORLD);
        MPI_Gather(&actual_threads_on_rank, 1, MPI_INT, num_actual_threads_on_rank.data(), 1, MPI_INT, HEAD_RANK, MPI_COMM_WORLD);

        // Exchange buffers
        double buf_par[nparams], buf_obj;
        if (mpi_rank == HEAD_RANK) {
            for (int other_rank = 0; other_rank < mpi_num_ranks; other_rank++) {
                if (other_rank == HEAD_RANK) continue;
                int num_sqs_results_of_rank = num_sqs_results[other_rank];
                /*
                 * The synchonization routing works the following
                 *  - we collect the actual number of found results (num_sqs_results_of_rank) form each rank (other_rank)
                 *  - for each result there are three Send/Recv
                 *    - 1. SRO parameters
                 *    - 2. the configuration
                 *    - 3. the objective function value
                 *  - we construct a new SQSResult object on the head rank
                 *  - after that we synchronize thread timing information
                 */
                for (int j = 0; j < num_sqs_results_of_rank; j++) {
                    MPI_Status status;
                    MPI_Recv(&buf_par[0], static_cast<int>(nparams), MPI_DOUBLE, other_rank, TAG_COLLECT, MPI_COMM_WORLD, &status);
                    MPI_Recv(&buf_conf[0], static_cast<int>(settings.num_atoms()), MPI_UINT8_T, other_rank, TAG_COLLECT, MPI_COMM_WORLD, &status);
                    MPI_Recv(&buf_obj, 1, MPI_DOUBLE, other_rank, TAG_COLLECT, MPI_COMM_WORLD, &status);
                    BOOST_LOG_TRIVIAL(trace) << "do_pair_iterations::rank::" << mpi_rank << "::got_result::" << j << "::from::" << other_rank;
                    SQSResult collected(buf_obj, {-1}, configuration_t(&buf_conf[0], &buf_conf[0]+settings.num_atoms()), parameter_storage_t(&buf_par[0], &buf_par[0] + nparams));
                    tmp_results.push_back(collected);
                }
                // Recieve also timing information from the other ranks
                MPI_Status status_recv_timing;
                int threads_on_other_rank = num_actual_threads_on_rank[other_rank];
                MPI_Recv(&buf_timings[0], threads_on_other_rank, MPI_DOUBLE, other_rank, TAG_COLLECT, MPI_COMM_WORLD, &status_recv_timing);
                BOOST_LOG_TRIVIAL(trace) << "do_pair_iterations::rank::" << mpi_rank << "::received_thread_timings::from::" << other_rank;
                thread_timings.emplace(other_rank, std::vector<double>(&buf_timings[0], &buf_timings[0]+threads_on_other_rank));
            }
        }
        else {
            // on the non-head ranks we send all the data stored in the rank-internal SQSResult
            for (auto &r : tmp_results) {
                std::copy(r.storage().begin(), r.storage().end(), buf_par);
                std::copy(r.configuration().begin(), r.configuration().end(), buf_conf);
                buf_obj = r.objective();
                MPI_Send(&buf_par[0], static_cast<int>(nparams), MPI_DOUBLE, HEAD_RANK, TAG_COLLECT, MPI_COMM_WORLD);
                MPI_Send(&buf_conf[0], static_cast<int>(settings.num_atoms()), MPI_UINT8_T, HEAD_RANK, TAG_COLLECT, MPI_COMM_WORLD);
                MPI_Send(&buf_obj, 1, MPI_DOUBLE, HEAD_RANK, TAG_COLLECT, MPI_COMM_WORLD);
            }
            // Send thread timing information over to head rank
            std::copy(thread_timings[mpi_rank].begin(), thread_timings[mpi_rank].end(), buf_timings);
            MPI_Send(&buf_timings[0], actual_threads_on_rank, MPI_DOUBLE, HEAD_RANK, TAG_COLLECT, MPI_COMM_WORLD);
        }
        if (!mpi_initialized) MPI_Finalize();
#endif
        std::unordered_set<rank_t> ranks;
        for (auto &r : tmp_results) {
            // rank computation is relatively demanding, in the main loop we only set it to {-1}
            rank_t rank = rank_permutation(r.configuration(), settings.num_species());
            r.set_rank(rank);
            r.set_configuration(settings.unpack_configuration(r.configuration()));
            BOOST_LOG_TRIVIAL(trace) << "do_pair_iterations::rank::" << mpi_rank << "::conf = " << format_sqs_result(r);
            if (settings.mode() == random && !ranks.insert(rank).second) {
                BOOST_LOG_TRIVIAL(debug) << "do_pair_iterations::rank::" << mpi_rank << "::duplicate_configuration = " << rank;
                continue;
            }
            else final_results.push_back(std::move(r));
        }
        BOOST_LOG_TRIVIAL(info) << "do_pair_iterations::rank::" << mpi_rank << "::num_results = " << final_results.size();
        return std::make_tuple(final_results, thread_timings);
    }

    SQSResult do_pair_analysis(const IterationSettings &settings) {
        log_settings("do_pair_analysis", settings);
        size_t nshells {settings.num_shells()},
                nspecies{settings.num_species()},
                nparams {nshells * nspecies * nspecies};
        configuration_t configuration(settings.packed_configuraton());
        std::vector<size_t> pair_list(convert_pair_list(settings.pair_list()));
        std::vector<size_t> hist(utils::configuration_histogram(settings.packed_configuraton()));

        parameter_storage_t targets_objectives {settings.target_objective()};
        parameter_storage_t parameter_weights {settings.parameter_weights()};
        parameter_storage_t parameter_prefactors {settings.parameter_prefactors()};
        parameter_storage_t sro_parameters(reduced_size * nshells);

        count_pairs(configuration, pair_list, sro_parameters, nspecies, true);
        double objective = calculate_pair_objective(sro_parameters, parameter_prefactors, parameter_weights, target_objectives);
        rank_t configuration_rank = rank_permutation(configuration, nspecies);
        SQSResult result(objective, configuration_rank, settings.unpack_configuration(configuration), sro_parameters);
        return result;
    }
}