//
// Created by dominik on 08.07.21.
//

#ifndef SQSGENERATOR_SETTINGS_HPP
#define SQSGENERATOR_SETTINGS_HPP

#include "types.hpp"
#include "atomistics.hpp"

using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator::utils {
    enum iteration_mode { random = 0, systematic = 1};

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
        array_2d_t m_parameter_weights;
        std::vector<AtomPair> m_pair_list;
        iteration_mode m_mode;

    public:


        IterationSettings(Structure &structure, double target_objective, array_2d_t parameter_weights,  pair_shell_weights_t shell_weights, int iterations, int output_configurations, iteration_mode mode = iteration_mode::random);
        const_pair_shell_matrix_ref_t shell_matrix(uint8_t prec);
        [[nodiscard]] std::vector<AtomPair> pair_list() const;
        [[nodiscard]] const Structure &structure() const;
        [[nodiscard]] size_t num_atoms() const;
        [[nodiscard]] int num_iterations() const;
        [[nodiscard]] size_t num_species() const;
        [[nodiscard]] size_t num_shells() const;
        [[nodiscard]] int num_output_configurations() const;
        [[nodiscard]] double target_objective() const;
        [[nodiscard]] pair_shell_weights_t shell_weights() const;
        [[nodiscard]] configuration_t packed_configuraton() const;
        [[nodiscard]] const_array_2d_ref_t parameter_weights() const;
        [[nodiscard]] iteration_mode mode() const;


    };
}

#endif //SQSGENERATOR_SETTINGS_HPP
