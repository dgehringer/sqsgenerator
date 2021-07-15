//
// Created by dominik on 08.07.21.
//

#ifndef SQSGENERATOR_SETTINGS_HPP
#define SQSGENERATOR_SETTINGS_HPP

#include "types.hpp"
#include "atomistics.hpp"

using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator {
    enum iteration_mode { random = 0, systematic = 1};

    class IterationSettings {

    private:
        Structure &m_structure;
        uint8_t m_prec;
        int m_niterations;
        int m_noutput_configurations;
        size_t m_nspecies;
        configuration_t m_configuration_packing_indices;
        configuration_t m_packed_configuration;
        pair_shell_weights_t m_shell_weights;
        iteration_mode m_mode;
        array_2d_t m_parameter_weights;
        array_3d_t m_target_objective;
        array_3d_t m_parameter_prefactors;
        void init_prefactors();

    public:

        IterationSettings(Structure &structure, const_array_3d_ref_t target_objective, const_array_2d_ref_t parameter_weights,  const pair_shell_weights_t &shell_weights, int iterations, int output_configurations, iteration_mode mode = iteration_mode::random, uint8_t prec = 5);
        const_pair_shell_matrix_ref_t shell_matrix();
        [[nodiscard]] size_t num_atoms() const;
        [[nodiscard]] size_t num_shells() const;
        [[nodiscard]] int num_iterations() const;
        [[nodiscard]] size_t num_species() const;
        [[nodiscard]] iteration_mode mode() const;
        [[nodiscard]] const Structure &structure() const;
        [[nodiscard]] int num_output_configurations() const;
        [[nodiscard]] std::vector<AtomPair> pair_list() const;
        [[nodiscard]] pair_shell_weights_t shell_weights() const;
        [[nodiscard]] configuration_t packed_configuraton() const;
        [[nodiscard]] const_array_2d_ref_t parameter_weights() const;
        [[nodiscard]] const_array_3d_ref_t target_objective() const;
        [[nodiscard]] const_array_3d_ref_t parameter_prefactors() const;
        [[nodiscard]] configuration_t unpack_configuration(const configuration_t &conf) const;
        [[nodiscard]] std::tuple<std::vector<shell_t>, std::vector<double>> shell_indices_and_weights() const;
    };
}

#endif //SQSGENERATOR_SETTINGS_HPP
