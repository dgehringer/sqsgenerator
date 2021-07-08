//
// Created by dominik on 08.07.21.
//

#ifndef SQSGENERATOR_SETTINGS_HPP
#define SQSGENERATOR_SETTINGS_HPP

#include "types.hpp"
#include "atomistics.hpp"

using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator::utils {

    class IterationSettings {

    private:
        int m_niterations;
        int m_noutput_configurations;
        size_t m_nspecies;
        double m_target_objective;
        Structure &m_structure;
        configuration_t m_configuration_packing_indices;
        configuration_t m_packed_configuration;
        pair_shell_weights_t m_shell_weights;
        parameter_storage_t m_parameter_weight_storage;
        std::vector<AtomPair> m_pair_list;

    public:

        IterationSettings(Structure &structure, double target_objective, parameter_storage_t parameter_weights,  pair_shell_weights_t shell_weights, int iterations, int output_configurations);

        const_pair_shell_matrix_ref_t shell_matrix(uint_t prec);
        [[nodiscard]] const std::vector<AtomPair>& pair_list() const;
        [[nodiscard]] const Structure &structure() const;
        [[nodiscard]] size_t num_atoms() const;
        [[nodiscard]] int num_iterations() const;
        [[nodiscard]] size_t num_species() const;
        [[nodiscard]] size_t num_shells() const;
        [[nodiscard]] int num_output_configurations() const;
        [[nodiscard]] double target_objective() const;
        pair_shell_weights_t &shell_weights() const;
        [[nodiscard]] configuration_t packed_configuraton() const;

        template<size_t NDims>
        boost::const_multi_array_ref<double, NDims> parameter_weights(const Shape<NDims> shape) const {
            return boost::const_multi_array_ref<double, NDims>(m_parameter_weight_storage.data(), shape);
        }
    };
}

#endif //SQSGENERATOR_SETTINGS_HPP
