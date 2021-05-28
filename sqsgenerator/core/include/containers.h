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
#include "moodycamel/concurrentqueue.h"

namespace sqsgenerator {

    class PairSQSResult {

    public:
        double objective;
        uint64_t rank;
        PairSROParameters parameters;
        Configuration configuration;
        PairSQSResult() {}
        PairSQSResult(double objective, uint64_t rank, const Configuration conf, const PairSROParameters parameters) : objective(objective), rank(rank), configuration(conf), parameters(parameters) {}
        PairSQSResult(double objective, const Configuration conf, const PairSROParameters parameters) : objective(objective), configuration(conf), parameters(parameters) {
            auto nspecies = unique_species(conf).size();
            rank = rank_permutation_std(conf, nspecies);
        }

        PairSQSResult(double objective, uint64_t rank, const Configuration conf, size_t nshells, size_t nspecies): objective(objective), configuration(conf),
                                                                                   parameters(boost::extents[nshells][nspecies][nspecies]), rank(rank) { }
        PairSQSResult(double objective, const Configuration conf, size_t nshells, size_t nspecies): PairSQSResult(objective, rank_permutation_std(conf, nspecies), conf, nshells, nspecies) {}
        PairSQSResult(double objective, const Configuration conf, size_t nshells): PairSQSResult(objective, conf, nshells, unique_species(conf).size()) {}
        PairSQSResult(const PairSQSResult &other) = default;


        PairSQSResult& operator=(const PairSQSResult&& other) {
            rank = other.rank;
            objective = other.objective;
            PairSROParameters::extent_gen extent;
            auto shape = other.parameters.shape();
            parameters.resize(extent[shape[0]][shape[1]][shape[2]]);
            parameters = std::move(other.parameters);
            configuration = std::move(other.configuration);
            return *this;
        }

    };

    class SQSResultCollection {

    public:
        SQSResultCollection(int maxSize): m_bestObjective(std::numeric_limits<double>::max()), m_size(0), m_q(maxSize > 0 ? static_cast<size_t>(maxSize): 6 * moodycamel::ConcurrentQueueDefaultTraits::BLOCK_SIZE), m_maxSize(maxSize) {
        }

        double bestObjective() const {
            return m_bestObjective;
        }

        bool addResult(const PairSQSResult &item) {
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
            }
            return false;
        }

        size_t size() const {
            return m_size;
        }

    private:
        moodycamel::ConcurrentQueue<PairSQSResult> m_q;
        std::atomic<size_t> m_size;
        std::atomic<double> m_bestObjective;
        std::mutex m_mutex_clear;
        int m_maxSize;

        void clearQueue() {
            std::cout << "clearQueue()" << std::endl;
            m_mutex_clear.lock();
            // Only one threa can clear the queue, this is important for m_q.apporx_size() to accomodate
            while(m_q.size_approx()) {
                PairSQSResult item;
                m_q.try_dequeue(item);
            }
            m_size = 0;
            m_mutex_clear.unlock();
        }
    };
}


#endif //SQSGENERATOR_CONTAINERS_H
