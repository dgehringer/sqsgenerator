//
// Created by dominik on 08.07.21.
//

#include "utils.hpp"
#include "settings.hpp"
#include <utility>

namespace sqsgenerator::utils {


    IterationSettings::IterationSettings(Structure &structure, double target_objective, array_2d_t parameter_weights,  pair_shell_weights_t shell_weights, int iterations, int output_configurations, iteration_mode mode, uint8_t prec) :
        m_mode(mode),
        m_prec(prec),
        m_structure(structure),
        m_niterations(iterations),
        m_target_objective(target_objective),
        m_parameter_weights(parameter_weights),
        m_shell_weights(std::move(shell_weights)),
        m_noutput_configurations(output_configurations),
        m_nspecies(unique_species(structure.configuration()).size())
        {

            std::tie(m_configuration_packing_indices, m_packed_configuration) = pack_configuration(structure.configuration());
            auto nshells {num_shells()};

            typedef array_3d_t::index index_t;
            m_parameter_prefactors.resize(boost::extents[nshells][m_nspecies][m_nspecies]);
            std::vector<shell_t> shells = std::get<0>(shell_indices_and_weights());
            std::map<shell_t, size_t> neighbor_count;
            for (const auto &shell: shells) neighbor_count.emplace(std::make_pair(shell, 0));
            auto shell_mat = shell_matrix();
            for (const_pair_shell_matrix_ref_t::index i = 1; i < num_atoms(); i++) {
                auto neighbor_shell {shell_mat[0][i]};
                if (neighbor_count.count(neighbor_shell)) neighbor_count[neighbor_shell]++;
            }
            auto hist = configuration_histogram(m_packed_configuration);
            for (index_t i = 0; i < nshells; i++) {
                double M_i {static_cast<double>(neighbor_count[shells[i]])};
                for (index_t a = 0; a < m_nspecies; a++) {
                    double x_a {static_cast<double>(hist[a])/num_atoms()};
                    for (index_t b = a; b < m_nspecies; b++) {
                        double x_b {static_cast<double>(hist[b])/num_atoms()};
                        double prefactor {1.0/(M_i*x_a*x_b)};
                        m_parameter_prefactors[i][a][b] = prefactor;
                        m_parameter_prefactors[i][b][a] = prefactor;
                    }
                }
            }
        }

    const_pair_shell_matrix_ref_t IterationSettings::shell_matrix() {
        return m_structure.shell_matrix(m_prec);
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

    [[nodiscard]] std::tuple<std::vector<shell_t>, std::vector<double>> IterationSettings::shell_indices_and_weights() const {
        std::vector<shell_t> shells;
        std::vector<double> sorted_shell_weights;
        for (const auto &weight: m_shell_weights) shells.push_back(weight.first);
        std::sort(shells.begin(), shells.end());
        for (const auto &shell: shells) sorted_shell_weights.push_back(m_shell_weights.at(shell));
        return std::make_tuple(shells, sorted_shell_weights);
    }

    [[nodiscard]] const_array_3d_ref_t IterationSettings::parameter_prefactors() const {
        return boost::make_array_ref<const_array_3d_ref_t>(m_parameter_prefactors);
    }


};