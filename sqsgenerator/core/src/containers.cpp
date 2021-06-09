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
    SQSResult::SQSResult(const SQSResult &&other) :
        m_objective(std::move(other.m_objective)),
        m_rank(std::move(other.m_rank)),
        m_configuration(std::move(other.m_configuration)),
        m_storage(std::move(other.m_storage)) {
        std::cout << "SQSResult.ctor (move) = (" << m_storage.size() << ")" << std::endl;
    };

    SQSResult &SQSResult::operator=(const SQSResult &&other) {
        m_rank = std::move(other.m_rank);
        m_objective = std::move(other.m_objective);
        m_configuration = std::move(other.m_configuration);
        m_storage = std::move(other.m_storage);
        std::cout << "Move assignment = parameters(" << m_storage.size() << ")" << std::endl;
        return *this;
    }

    const Configuration& SQSResult::get_configuration() const {
        return m_configuration;
    }
    /*
    template<size_t NDims>
    boost::const_multi_array_ref<double, NDims> SQSResult::getParameters(const boost::detail::multi_array::extent_gen<NDims> &shp) const
    {

    }*/


}