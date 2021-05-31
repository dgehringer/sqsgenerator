//
// Created by dominik on 28.05.21.
//

#ifndef SQSGENERATOR_CONTAINERS_H
#define SQSGENERATOR_CONTAINERS_H

#include <iostream>
#include <types.h>
#include <gmpxx.h>
#include <stdint.h>
#include <rank.h>
#include <atomic>
#include <limits>
#include <thread>
#include <mutex>
#include <type_traits>
#include "utils.h"
#include "moodycamel/concurrentqueue.h"

namespace sqsgenerator {

    template<typename T>
    class SQSResult {

    public:
        double objective;
        uint64_t rank;
        T parameters;
        Configuration configuration;
        SQSResult() {}
        SQSResult(double objective, uint64_t rank, const Configuration conf, const T params) : objective(objective), rank(rank), configuration(conf), parameters(params) {
            std::cout << "SQSResult.ctor (default) = parameters(" << parameters.num_elements() << ")" << std::endl;
        }
        SQSResult(double objective, const Configuration conf, const T parameters) : objective(objective), configuration(conf), parameters(parameters) {
            auto nspecies = unique_species(conf).size();
            rank = rank_permutation_std(conf, nspecies);
        }

        SQSResult(const SQSResult &other) : objective(other.objective), rank(other.rank), configuration(other.configuration){
            boost::extentTo(parameters, other.parameters);
            parameters = other.parameters;
            std::cout << "SQSResult.ctor (copy) = parameters(" << parameters.num_elements() << ")" << std::endl;
        };

        SQSResult(const SQSResult &&other) : objective(other.objective), rank(other.rank), configuration(other.configuration.size()){
            boost::extentTo(parameters, other.parameters);
            configuration = std::move(other.configuration);
            parameters = std::move(other.parameters);
            std::cout << "SQSResult.ctor (move) = (" << parameters.num_elements() << ", " << other.parameters.num_elements() << ")" << std::endl;
        };

        SQSResult& operator=(const SQSResult& other) = default;

        SQSResult& operator=(const SQSResult&& other) {
            rank = other.rank;
            objective = other.objective;
            boost::extentTo(parameters, other.parameters);
            parameters = std::move(other.parameters);
            configuration = std::move(other.configuration);
            std::cout << "Move assignment = parameters(" << parameters.num_elements() << ")" << std::endl;
            return *this;
        }

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

    typedef SQSResult<PairSROParameters> PairSQSResult;
    typedef SQSResult<TripletSROParameters> TripletSQSResult;
    typedef SQSResultCollection<PairSQSResult> PairSQSIterationResult;
    typedef SQSResultCollection<TripletSQSResult> TripletSQSIterationResult;
}


#endif //SQSGENERATOR_CONTAINERS_H
