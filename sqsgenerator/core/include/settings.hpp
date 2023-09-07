//
// Created by dominik on 08.07.21.
//

#ifndef SQSGENERATOR_SETTINGS_HPP
#define SQSGENERATOR_SETTINGS_HPP

#include <functional>
#include "types.hpp"
#include "result.hpp"
#include "atomistics.hpp"

using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator {
    enum iteration_mode { random = 0, systematic = 1};

    typedef std::function<bool(rank_t, const SQSResult&, int, int)> callback_t;
    typedef std::map<std::string, std::vector<callback_t>> callback_map_t;

    static const callback_map_t DEFAULT_CALLBACKS {
            {"found_better_or_equal", {}},
            {"found_better", {}}
    };

    class IterationSettings {

    private:
        Structure m_structure;
        composition_t m_composition;
        double m_atol;
        double m_rtol;
        rank_t m_niterations;
        int m_noutput_configurations;
        size_t m_nspecies;
        configuration_t m_configuration_packing_indices;
        configuration_t m_packed_configuration;
        pair_shell_weights_t m_shell_weights;
        iteration_mode m_mode;
        array_3d_t m_parameter_weights;
        array_3d_t m_target_objective;
        pair_shell_matrix_t m_shell_matrix;
        array_3d_t m_parameter_prefactors;
        std::vector<shell_t> m_available_shells;
        std::vector<double> m_shell_distances;
        std::vector<int> m_threads_per_rank;
        arrangement_t m_arrange_forward;
        arrangement_t m_arrange_backward;
        shuffling_bounds_t m_shuffling_bounds;
        callback_map_t m_callback_map;

    public:

        IterationSettings(
                Structure structure,
                composition_t composition,
                const_array_3d_ref_t target_objective,
                const_array_3d_ref_t parameter_weights,
                const_array_3d_ref_t bond_count_prefactors,
                pair_shell_weights_t shell_weights,
                rank_t iterations,
                int output_configurations,
                std::vector<double> shell_distances,
                std::vector<int> threads_per_rank,
                double atol=ATOL,
                double rtol=RTOL,
                iteration_mode mode = iteration_mode::random,
                callback_map_t callback_map = DEFAULT_CALLBACKS);


        [[nodiscard]] double atol() const;
        [[nodiscard]] double rtol() const;
        [[nodiscard]] size_t num_atoms() const;
        [[nodiscard]] size_t num_shells() const;
        [[nodiscard]] size_t num_species() const;
        [[nodiscard]] iteration_mode mode() const;
        [[nodiscard]] rank_t num_iterations() const;
        [[nodiscard]] composition_t composition() const;
        [[nodiscard]] const Structure &structure() const;
        [[nodiscard]] callback_map_t callback_map() const;
        [[nodiscard]] int num_output_configurations() const;
        [[nodiscard]] arrangement_t arrange_forward() const;
        [[nodiscard]] arrangement_t arrange_backward() const;
        [[nodiscard]] std::vector<AtomPair> pair_list() const;
        [[nodiscard]] std::vector<int> threads_per_rank() const;
        [[nodiscard]] pair_shell_weights_t shell_weights() const;
        [[nodiscard]] configuration_t packed_configuraton() const;
        [[nodiscard]] shuffling_bounds_t shuffling_bounds() const;
        [[nodiscard]] std::vector<double> shell_distances() const;
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
