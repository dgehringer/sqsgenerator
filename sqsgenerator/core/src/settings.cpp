//
// Created by dominik on 08.07.21.
//

#include "utils.hpp"
#include "settings.hpp"
#include <utility>

namespace sqsgenerator::utils {


    IterationSettings::IterationSettings(Structure &structure, double target_objective, array_2d_t parameter_weights,  pair_shell_weights_t shell_weights, int iterations, int output_configurations, iteration_mode mode) :
        m_structure(structure),
        m_niterations(iterations),
        m_noutput_configurations(output_configurations),
        m_parameter_weights(parameter_weights),
        m_shell_weights(std::move(shell_weights)),
        m_target_objective(target_objective),
        m_mode(mode),
        m_nspecies(unique_species(structure.configuration()).size())
        {
            std::tie(m_configuration_packing_indices, m_packed_configuration) = pack_configuration(structure.configuration());
        }

    const_pair_shell_matrix_ref_t IterationSettings::shell_matrix(uint8_t prec) {
        return m_structure.shell_matrix(prec);
    }

    [[nodiscard]] std::vector<AtomPair> IterationSettings::pair_list() const {
        return m_structure.create_pair_list(m_shell_weights);
    }

    [[nodiscard]] const Structure& IterationSettings::structure() const {
        return m_structure;
    }

    [[nodiscard]] size_t IterationSettings::num_atoms() const {
        return m_structure.num_atoms();
    }

    [[nodiscard]] int IterationSettings::num_iterations() const {
        return m_niterations;
    }

    [[nodiscard]] size_t IterationSettings::num_species() const {
        return m_nspecies;
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

    pair_shell_weights_t IterationSettings::shell_weights() const {
        return m_shell_weights;
    }

    [[nodiscard]] configuration_t IterationSettings::packed_configuraton() const {
        return m_packed_configuration;
    }

    [[nodiscard]] const_array_2d_ref_t IterationSettings::parameter_weights() const {
        return boost::make_array_ref<const_array_2d_ref_t>(m_parameter_weights);
    }

    iteration_mode IterationSettings::mode() const {
        return m_mode;
    }


};