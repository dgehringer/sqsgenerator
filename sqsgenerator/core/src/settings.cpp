//
// Created by dominik on 08.07.21.
//

#include "settings.hpp"

namespace sqsgenerator::utils {


    IterationSettings::IterationSettings(Structure &structure, double target_objective, parameter_storage_t parameter_weights,  pair_shell_weights_t shell_weights, int iterations, int output_configurations) :
        m_structure(structure),
        m_niterations(iterations),
        m_noutput_configurations(output_configurations),
        m_parameter_weight_storage(parameter_weights),
        m_shell_weights(shell_weights),
        m_target_objective(target_objective)
        {
        }

    const_pair_shell_matrix_ref_t IterationSettings::shell_matrix(uint_t prec) {
        return m_structure.shell_matrix(prec);
    }

    [[nodiscard]] const std::vector<AtomPair>& IterationSettings::pair_list() const {
        return m_pair_list;
    }

    [[nodiscard]] const Structure& IterationSettings::structure() const {
        return m_structure;
    }

    [[nodiscard]] size_t IterationSettings::num_atoms() const {
        return m_structure.num_atoms();
    }

    [[nodiscard]] int IterationSettings::num_iterations() const {
        return m_iterations;
    }

    [[nodiscard]] size_t IterationSettings::num_species() const {
        return m_configuration_packing_indices.size();
    }

    [[nodiscard]] size_t IterationSettings::num_shells() const {
        return m_shell_weights.size();
    }

    [[nodiscard]] int IterationSettings::num_output_configurations() const {
        return m_noutput_configurations;
    }

    [[nodiscard]] double IterationSettings::target_objective() const {
        return m_target_objective;
    }

    pair_shell_weights_t& IterationSettings::shell_weights() const {
        return m_shell_weights;
    }

    [[nodiscard]] configuration_t IterationSettings::packed_configuraton() const {
        return m_packed_configuration;
    }

};