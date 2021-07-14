//
// Created by dominik on 14.07.21.
//

#ifndef SQSGENERATOR_RESULT_HPP
#define SQSGENERATOR_RESULT_HPP
#include "types.hpp"

namespace sqsgenerator {

    class SQSResult {
    private:
        double m_objective;
        rank_t m_rank;
        configuration_t m_configuration;
        parameter_storage_t m_storage;

    public:

        SQSResult();

        SQSResult(double objective, rank_t rank, configuration_t conf, parameter_storage_t params);

        SQSResult(double objective, configuration_t conf, parameter_storage_t parameters);

        //TODO: we could write: SQSResult(const SQSResult &other) = default; here
        SQSResult(const SQSResult &other);

        SQSResult(SQSResult &&other) noexcept;

        SQSResult &operator=(const SQSResult &other) = default;

        SQSResult &operator=(SQSResult &&other) noexcept;

        void set_rank(rank_t rank);

        void set_configuration(configuration_t conf);

        [[nodiscard]] double objective() const;

        [[nodiscard]] const configuration_t &configuration() const;

        [[nodiscard]] rank_t rank() const;

        [[nodiscard]] const parameter_storage_t &storage() const;

        template<size_t NDims>
        boost::const_multi_array_ref<double, NDims> parameters(const Shape <NDims> shape) const {
            return boost::const_multi_array_ref<double, NDims>(m_storage.data(), shape);
        }
    };

}

#endif //SQSGENERATOR_RESULT_HPP
