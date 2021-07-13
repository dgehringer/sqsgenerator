//
// Created by dominik on 28.05.21.
//

#ifndef SQSGENERATOR_CONTAINERS_HPP
#define SQSGENERATOR_CONTAINERS_HPP

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
        configuration_t m_configuration;
        parameter_storage_t m_storage;

    public:

        SQSResult();
        SQSResult(double objective, cpp_int rank, configuration_t conf, parameter_storage_t params);
        SQSResult(double objective, configuration_t conf, parameter_storage_t parameters);
        //TODO: we could write: SQSResult(const SQSResult &other) = default; here
        SQSResult(const SQSResult &other);
        SQSResult(SQSResult &&other) noexcept;

        SQSResult& operator=(const SQSResult& other) = default;
        SQSResult& operator=(SQSResult&& other) noexcept;

        void set_rank(rank_t rank);
        void set_configuration(configuration_t conf);
        [[nodiscard]] double objective() const;
        [[nodiscard]] const configuration_t& configuration() const;
        [[nodiscard]] rank_t rank() const;
        [[nodiscard]] const parameter_storage_t& storage() const;
        template<size_t NDims>
        boost::const_multi_array_ref<double, NDims> parameters(const Shape<NDims> shape) const {
            return boost::const_multi_array_ref<double, NDims>(m_storage.data(), shape);
        }
    };



}


#endif //SQSGENERATOR_CONTAINERS_HPP
