//
// Created by dominik on 08.07.21.
//

#ifndef SQSGENERATOR_SETTINGS_HPP
#define SQSGENERATOR_SETTINGS_HPP

#include "types.hpp"
#include "atomistics.hpp"

using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator::utils {

    template<typename ShellWeight>
    class IterationSettings {

    private:
        int m_iterations;
        int m_noutput_configurations;
        size_t m_nspecies;
        double m_target_objective;
        Structure m_structure;
        configuration_t m_configuration_packing_indices;
        configuration_t m_packed_configuration;
        ShellWeight m_shell_weights;
        std::vector<double> m_parameter_weight_storage;
        std::vector<AtomPair> m_pair_list;

    public:
        const std::vector<AtomPair>& pair_list() const {
            return m_pair_list;
        }

        const Structure &structure() const {
            return m_structure;
        }

        size_t num_atoms() const {
            return m_structure.num_atoms();
        }

        int num_iterations() const {
            return m_iterations;
        }

        size_t num_species() const {
            return m_configuration_packing_indices.size();
        }

        size_t num_shells() const {
            return m_shell_weights.size();
        }

        int num_output_configurations() const {
            return m_noutput_configurations;
        }

        double target_objective() const {
            return m_target_objective;
        }

        ShellWeight &shell_weights() const {
            return m_shell_weights;
        }

        [[nodiscard]] configuration_t packed_configuraton() const {
            return m_packed_configuration;
        }

        template<size_t NDims>
        boost::const_multi_array_ref<double, NDims> parameter_weights(const Shape<NDims> shape) const {
            return boost::const_multi_array_ref<double, NDims>(m_parameter_weight_storage.data(), shape);
        }

    };
}

#endif //SQSGENERATOR_SETTINGS_HPP
