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
        composition_t m_composition;
        double m_atol;
        double m_rtol;
        int m_niterations;
        int m_noutput_configurations;
        size_t m_nspecies;
        configuration_t m_configuration_packing_indices;
        configuration_t m_packed_configuration;
        pair_shell_weights_t m_shell_weights;
        iteration_mode m_mode;
        array_3d_t m_parameter_weights;
        array_3d_t m_target_objective;
        array_3d_t m_parameter_prefactors;
        std::vector<shell_t> m_available_shells;
        std::vector<double> m_shell_distances;
        std::vector<int> m_threads_per_rank;
        pair_shell_matrix_t m_shell_matrix;
        arrangement_t m_arrange_forward;
        arrangement_t m_arrange_backward;
        shuffling_bounds_t m_shuffling_bounds;
        void init_prefactors();

    public:

        IterationSettings(
                Structure &structure,
                composition_t composition,
                const_array_3d_ref_t target_objective,
                const_array_3d_ref_t parameter_weights,
                pair_shell_weights_t shell_weights,
                int iterations,
                int output_configurations,
                std::vector<double> shell_distances,
                std::vector<int> threads_per_rank,
                double atol=1.0e-5,
                double rtol=1.05e-8,
                iteration_mode mode = iteration_mode::random);

        IterationSettings(
                Structure &structure,
                composition_t composition,
                const_array_3d_ref_t target_objective,
                const_array_3d_ref_t parameter_weights,
                pair_shell_weights_t shell_weights,
                int iterations,
                int output_configurations,
                std::vector<int> threads_per_rank,
                double atol=1.0e-5,
                double rtol=1.05e-8,
                iteration_mode mode = iteration_mode::random);

        [[nodiscard]] double atol() const;
        [[nodiscard]] double rtol() const;
        [[nodiscard]] size_t num_atoms() const;
        [[nodiscard]] size_t num_shells() const;
        [[nodiscard]] int num_iterations() const;
        [[nodiscard]] size_t num_species() const;
        [[nodiscard]] iteration_mode mode() const;
        [[nodiscard]] composition_t composition() const;
        [[nodiscard]] const Structure &structure() const;
        [[nodiscard]] int num_output_configurations() const;
        [[nodiscard]] arrangement_t arrange_forward() const;
        [[nodiscard]] arrangement_t arrange_backward() const;
        [[nodiscard]] std::vector<AtomPair> pair_list() const;
        [[nodiscard]] std::vector<int> threads_per_rank() const;
        [[nodiscard]] pair_shell_weights_t shell_weights() const;
        [[nodiscard]] configuration_t packed_configuraton() const;
        [[nodiscard]] shuffling_bounds_t shuffling_bounds() const;
        [[nodiscard]] std::vector<shell_t> available_shells() const;
        [[nodiscard]] const_array_3d_ref_t parameter_weights() const;
        [[nodiscard]] const_array_3d_ref_t target_objective() const;
        [[nodiscard]] const_array_3d_ref_t parameter_prefactors() const;
        [[nodiscard]] const_pair_shell_matrix_ref_t shell_matrix() const;
        [[nodiscard]] configuration_t unpack_configuration(const configuration_t &conf) const;
        [[nodiscard]] std::tuple<std::vector<shell_t>, std::vector<double>> shell_indices_and_weights() const;
    };
}

#endif //SQSGENERATOR_SETTINGS_HPP
