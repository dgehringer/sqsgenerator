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
        SQSResult(const SQSResult &&other);

        SQSResult& operator=(const SQSResult& other) = default;
        SQSResult& operator=(const SQSResult&& other);

        double getObjective() const;
        const Configuration& getConfiguration() const;

        template<size_t NDims>
        boost::const_multi_array_ref<double, NDims> getParameters(const boost::detail::multi_array::extent_gen<NDims> &shape) const;
    };

    template<typename T>
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
        SQSResultCollection(int maxSize):
                m_size(0),
                m_maxSize(maxSize),
                m_bestObjective(std::numeric_limits<double>::max()),
                m_q(maxSize > 0 ? static_cast<size_t>(maxSize) : 6 * moodycamel::ConcurrentQueueDefaultTraits::BLOCK_SIZE),
                m_r() {}

        SQSResultCollection(const SQSResultCollection& other) = delete;

        SQSResultCollection(const SQSResultCollection&& other) :
                m_size(std::move(other.m_size)),
                m_maxSize(std::move(other.m_maxSize)),
                m_bestObjective(std::move(other.m_bestObjective)),
                m_q(std::move(other.m_q)),
                m_r(std::move(other.m_r)){
            std::cout << "SQSResultCollection.ctor (move)" << std::endl;
        }

        double bestObjective() const {
            return m_bestObjective;
        }

        bool addResult(const T &item) {
            std::cout << "SQSResultCollection.addResult()" << std::endl;
            if (item.objective > m_bestObjective) return false;
            else if (item.objective < m_bestObjective) {
                // we have to clear and remove all elements in the queue
                T copy = item;
                clearQueue();
                assert(m_q.size_approx() == 0);
                bool success  = m_q.try_enqueue(item);
                if (success) {
                    m_size += 1;
                    m_bestObjective = item.objective;
                }
                return success;
            }
            else {
                // We do not rotate configurations
                if (m_size >= m_maxSize) return false;
                else {
                    bool success = m_q.try_enqueue(item);
                    if (success) m_size += 1;
                    return success;
                }
            }
        }

        void collect() {
            std::cout << "SQSResultCollection.collect()" << std::endl;
            m_mutex_clear.lock();
            T item;
            while(m_q.size_approx()) {
                if(m_q.try_dequeue(item)) {
                    // we were successful. We move it on into the results vector
                    m_r.push_back(std::move(item));
                }
            }
            m_size = 0;
            m_mutex_clear.unlock();
        }

        size_t queueSize() const {
            return m_size;
        }

        size_t resultSize() const {
            return m_r.size();
        }

        const std::vector<T>& results() const {
            return m_r;
        }

    private:
        moodycamel::ConcurrentQueue<T> m_q;
        std::atomic<size_t> m_size;
        std::atomic<double> m_bestObjective;
        std::mutex m_mutex_clear;
        std::vector<T> m_r;
        int m_maxSize;

        void clearQueue() {
            std::cout << "SQSResultCollection.clearQueue()" << std::endl;
            m_mutex_clear.lock();
            // Only one thread can clear the queue, this is important for m_q.size_approx() since the queue, needs
            // be stabilized
            T item;
            while(m_q.size_approx()) {
                m_q.try_dequeue(item);
            }
            m_size = 0;
            m_mutex_clear.unlock();
        }
    };

}


#endif //SQSGENERATOR_CONTAINERS_H
