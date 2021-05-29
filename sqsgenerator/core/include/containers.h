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
        SQSResult(double objective, uint64_t rank, const Configuration conf, const T parameters) : objective(objective), rank(rank), configuration(conf), parameters(parameters) {
            std::cout << "Constructor = parameters(" << parameters.num_elements() << ")" << std::endl;
        }
        SQSResult(double objective, const Configuration conf, const T parameters) : objective(objective), configuration(conf), parameters(parameters) {
            auto nspecies = unique_species(conf).size();
            rank = rank_permutation_std(conf, nspecies);
        }

        SQSResult(const SQSResult &other) : objective(other.objective), rank(other.rank), configuration(other.configuration){
            boost::extentTo(parameters, other.parameters);
            parameters = other.parameters;
        };

        SQSResult& operator=(const SQSResult&& other) {
            rank = other.rank;
            objective = other.objective;
            boost::extentTo(parameters, other.parameters);
            parameters = std::move(other.parameters);
            configuration = std::move(other.configuration);
            return *this;
        }

    };

    template<typename T>
    class SQSResultCollection {

    public:
        SQSResultCollection(int maxSize): m_bestObjective(std::numeric_limits<double>::max()), m_size(0), m_q(maxSize > 0 ? static_cast<size_t>(maxSize): 6 * moodycamel::ConcurrentQueueDefaultTraits::BLOCK_SIZE), m_maxSize(maxSize) {}
        double bestObjective() const {
            return m_bestObjective;
        }

        bool addResult(const T &item) {
            if (item.objective > m_bestObjective) return false;
            else if (item.objective < m_bestObjective) {
                // we have to clear and remove all elements in the queue
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

        size_t size() const {
            return m_size;
        }

    private:
        moodycamel::ConcurrentQueue<T> m_q;
        std::atomic<size_t> m_size;
        std::atomic<double> m_bestObjective;
        std::mutex m_mutex_clear;
        int m_maxSize;

        void clearQueue() {
            std::cout << "clearQueue()" << std::endl;
            m_mutex_clear.lock();
            // Only one threa can clear the queue, this is important for m_q.size_approx() since the queue, needs
            // be stabilized
            while(m_q.size_approx()) {
                T item;
                m_q.try_dequeue(item);
            }
            m_size = 0;
            m_mutex_clear.unlock();
        }
    };

    typedef SQSResult<PairSROParameters> PairSQSResult;
}


#endif //SQSGENERATOR_CONTAINERS_H
