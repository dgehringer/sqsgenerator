//
// Created by dominik on 28.05.21.
//

#include "containers.hpp"

namespace sqsgenerator {

    SQSResult::SQSResult() {}

    SQSResult::SQSResult(double objective, cpp_int rank, const Configuration conf, const ParameterStorage params) :
        m_objective(objective),
        m_rank(rank),
        m_configuration(conf),
        m_storage(params) {
        std::cout << "SQSResult.ctor (default) = parameters(" << params.size() << ")" << std::endl;
    }

    SQSResult::SQSResult(double objective, const Configuration conf, const ParameterStorage parameters) :
        m_objective(objective),
        m_configuration(conf),
        m_storage(parameters) {
        auto nspecies = unique_species(conf).size();
        m_rank = rank_permutation(conf, nspecies);
    }

    //TODO: we could write: SQSResult(const SQSResult &other) = default; here
    SQSResult::SQSResult(const SQSResult &other) :
        m_objective(other.m_objective),
        m_rank(other.m_rank),
        m_configuration(other.m_configuration),
        m_storage(other.m_storage) {
        std::cout << "SQSResult.ctor (copy) = parameters(" << m_storage.size() << ")" << std::endl;
    };

    // Move constructor
    SQSResult::SQSResult(SQSResult &&other) noexcept :
        m_objective(other.m_objective),
        m_rank(std::move(other.m_rank)),
        m_configuration(std::move(other.m_configuration)),
        m_storage(std::move(other.m_storage)) {
        std::cout << "SQSResult.ctor (move) = (" << m_storage.size() << ")" << std::endl;
    };

    SQSResult& SQSResult::operator=(SQSResult&& other) noexcept  {
        m_rank = std::move(other.m_rank);
        m_objective = other.m_objective;
        m_configuration = std::move(other.m_configuration);
        m_storage = std::move(other.m_storage);
        std::cout << "Move assignment = parameters(" << m_storage.size() << ")" << std::endl;
        return *this;
    }

    cpp_int SQSResult::rank() const {
        return m_rank;
    }

    double SQSResult::objective() const {
        return m_objective;
    }

    const Configuration& SQSResult::configuration() const {
        return m_configuration;
    }

    const ParameterStorage &SQSResult::storage() const {
        return m_storage;
    }


    SQSResultCollection::SQSResultCollection(int maxSize):
            m_size(0),
            m_max_size(maxSize),
            m_best_objective(std::numeric_limits<double>::max()),
            m_q(maxSize > 0 ? static_cast<size_t>(maxSize) : 6 * moodycamel::ConcurrentQueueDefaultTraits::BLOCK_SIZE),
            m_r() {}


    SQSResultCollection::SQSResultCollection(SQSResultCollection&& other) noexcept :
            m_size(other.m_size.load()),
            m_max_size(other.m_max_size),
            m_best_objective(other.m_best_objective.load()),
            m_r(std::move(other.m_r)),
            m_q(std::move(other.m_q))
            {
        std::cout << "SQSResultCollection.ctor (move)" << std::endl;

    }

    bool SQSResultCollection::add_result(const SQSResult &item) {
        std::cout << "SQSResultCollection.add_result()" << std::endl;
        if (item.objective() > m_best_objective) return false;
        else if (item.objective() < m_best_objective) {
            // we have to clear and remove all elements in the queue
            clear_queue();
            assert(m_q.size_approx() == 0);
            bool success  = m_q.try_enqueue(item);
            if (success) {
                m_size += 1;
                m_best_objective = item.objective();
            }
            return success;
        }
        else {
            // We do not rotate configurations
            if (m_size >= m_max_size) return false;
            else {
                bool success = m_q.try_enqueue(item);
                if (success) m_size += 1;
                return success;
            }
        }
    }

    void SQSResultCollection::collect() {
        std::cout << "SQSResultCollection.collect()" << std::endl;
        m_mutex_clear.lock();
        SQSResult item;
        while(m_q.size_approx()) {
            if(m_q.try_dequeue(item)) {
                // we were successful. We move it on into the results vector
                m_r.push_back(std::move(item));
            }
        }
        m_size = 0;
        m_mutex_clear.unlock();
    }

    double SQSResultCollection::best_objective() const {
        return m_best_objective;
    }

    size_t SQSResultCollection::queue_size() const {
        return m_size;
    }

    size_t SQSResultCollection::size() const {
        return m_r.size();
    }

    const std::vector<SQSResult>& SQSResultCollection::results() const {
        return m_r;
    }

    size_t SQSResultCollection::result_size() const {
        return m_r.size();
    }

    void SQSResultCollection::clear_queue() {
        std::cout << "SQSResultCollection.clear_queue()" << std::endl;
        m_mutex_clear.lock();
        // Only one thread can clear the queue, this is important for m_q.size_approx() since the queue, needs
        // be stabilized
        SQSResult item;
        while(m_q.size_approx()) {
            m_q.try_dequeue(item);
        }
        m_size = 0;
        m_mutex_clear.unlock();
        std::cout << "SQSResultCollection.clear_queue()::end" << std::endl;
    }

}