//
// Created by dominik on 14.07.21.
//
#include "result.hpp"
#include "utils.hpp"
#include "rank.hpp"

namespace sqsgenerator {

    SQSResult::SQSResult() {}

    SQSResult::SQSResult(double objective, rank_t rank, const configuration_t conf, const parameter_storage_t params) :
            m_objective(objective),
            m_rank(std::move(rank)),
            m_configuration(conf),
            m_storage(params) {
    }

    SQSResult::SQSResult(double objective, const configuration_t conf, const parameter_storage_t parameters) :
            m_objective(objective),
            m_configuration(conf),
            m_storage(parameters) {
        auto nspecies = sqsgenerator::utils::unique_species(conf).size();
        m_rank = sqsgenerator::utils::rank_permutation(conf, nspecies);
    }

    //TODO: we could write: SQSResult(const SQSResult &other) = default; here
    SQSResult::SQSResult(const SQSResult &other) :
            m_objective(other.m_objective),
            m_rank(other.m_rank),
            m_configuration(other.m_configuration),
            m_storage(other.m_storage) {
    };

    // Move constructor
    SQSResult::SQSResult(SQSResult &&other) noexcept :
            m_objective(other.m_objective),
            m_rank(std::move(other.m_rank)),
            m_configuration(std::move(other.m_configuration)),
            m_storage(std::move(other.m_storage)) {
    };

    SQSResult& SQSResult::operator=(SQSResult&& other) noexcept  {
        m_rank = std::move(other.m_rank);
        m_objective = other.m_objective;
        m_configuration = std::move(other.m_configuration);
        m_storage = std::move(other.m_storage);
        return *this;
    }

    rank_t SQSResult::rank() const {
        return m_rank;
    }

    double SQSResult::objective() const {
        return m_objective;
    }

    const configuration_t & SQSResult::configuration() const {
        return m_configuration;
    }

    const parameter_storage_t &SQSResult::storage() const {
        return m_storage;
    }

    void SQSResult::set_rank(rank_t rank) {
        m_rank = std::move(rank);
    }

    void SQSResult::set_configuration(configuration_t conf){
        m_configuration = std::move(conf);
    }

}