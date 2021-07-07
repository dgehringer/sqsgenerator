//
// Created by dominik on 28.05.21.
//

#ifndef SQSGENERATOR_CONTAINERS_H
#define SQSGENERATOR_CONTAINERS_H

#include <iostream>
#include <atomic>
#include <vector>
#include <limits>
#include <thread>
#include <mutex>
#include <type_traits>
#include "types.hpp"
#include "rank.hpp"
#include "utils.hpp"
#include "moodycamel/concurrentqueue.h"
#include <boost/multiprecision/cpp_int.hpp>

using namespace sqsgenerator::utils;

namespace sqsgenerator {

    class SQSResult {
    private:
        double m_objective;
        cpp_int m_rank;
        Configuration m_configuration;
        ParameterStorage m_storage;

    public:

        SQSResult();
        SQSResult(double objective, cpp_int rank, const Configuration conf, const ParameterStorage params);
        SQSResult(double objective, const Configuration conf, const ParameterStorage parameters);
        //TODO: we could write: SQSResult(const SQSResult &other) = default; here
        SQSResult(const SQSResult &other);
        SQSResult(SQSResult &&other) noexcept;

        SQSResult& operator=(const SQSResult& other) = default;
        SQSResult& operator=(SQSResult&& other) noexcept;

        double objective() const;
        const Configuration& configuration() const;
        cpp_int rank() const;
        const ParameterStorage& storage() const;
        template<size_t NDims>
        boost::const_multi_array_ref<double, NDims> parameters(const Shape<NDims> shape) const {
            return boost::const_multi_array_ref<double, NDims>(m_storage.data(), shape);
        }
    };


    class SQSResultCollection {
        /*
         * Intended use of SQSResultCollection
         *
         * ======== serial ========
         * SQSResultCollection results(size);
         * ======= parallel =======
         * results.addResult(); # thread-safe
         * double best = results.bestObjective(); # thread-safe
         * ======== serial ========
         * results.collect() # Gather the computed results from the MPMCQueue
         *                   # thread-safe (but should not be called in a "parallel section"
         */

    public:
        SQSResultCollection(int maxSize);
        //SQSResultCollection(const SQSResultCollection& other) = delete;
        SQSResultCollection(SQSResultCollection&& other) noexcept;

        double best_objective() const;
        bool add_result(const SQSResult &item);
        void collect();
        size_t size() const;
        size_t queue_size() const;
        size_t result_size() const;
        const std::vector<SQSResult>& results() const;

    private:
        moodycamel::ConcurrentQueue<SQSResult> m_q;
        std::atomic<size_t> m_size;
        std::atomic<double> m_best_objective;
        std::mutex m_mutex_clear;
        std::vector<SQSResult> m_r;
        int m_max_size;

        void clear_queue();
    };


}


#endif //SQSGENERATOR_CONTAINERS_H
