//
// Created by dominik on 08.07.21.
//
#include "settings.hpp"
#include <cassert>
#include <utility>
#include <stdexcept>


using namespace sqsgenerator::utils;

namespace sqsgenerator {

    typedef array_3d_t::index index_t;

    Structure build_structure_from_composition(const Structure &structure, const composition_t &composition) {
        auto [arrange_forward, _, configuration, __] =  build_configuration(structure.configuration(), composition);
        return structure.with_species(configuration).rearranged(arrange_forward);
    }

    IterationSettings::IterationSettings(
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
            double atol,
            double rtol,
            iteration_mode mode) :
        m_structure(build_structure_from_composition(structure, composition)),
        m_composition(composition),
        m_atol(atol),
        m_rtol(rtol),
        m_niterations(iterations),
        m_noutput_configurations(output_configurations),
        m_nspecies(unique_species(m_structure.configuration()).size()),
        m_mode(mode),
        m_parameter_weights(parameter_weights),
        m_target_objective(target_objective),
        m_parameter_prefactors(bond_count_prefactors),
        m_shell_distances(shell_distances),
        m_threads_per_rank(threads_per_rank)
    {

        m_shell_matrix.resize(boost::extents[num_atoms()][num_atoms()]);
        m_shell_matrix = m_structure.shell_matrix(m_shell_distances, m_atol, m_rtol);

        auto num_elements {num_atoms() * num_atoms()};
        std::set<shell_t> unique(m_shell_matrix.data(), m_shell_matrix.data() + num_elements);
        m_available_shells = std::vector<shell_t>(unique.begin(), unique.end());
        std::sort(m_available_shells.begin(), m_available_shells.end());
        for (const auto &s : m_available_shells) {
            if (shell_weights.count(s)) m_shell_weights.emplace(std::make_pair(s, shell_weights.at(s)));
        }
        if (m_shell_weights.empty()) throw std::invalid_argument("None of the shells you have specified are available");
        configuration_t configuration;
        std::tie(m_arrange_forward, m_arrange_backward, configuration, m_shuffling_bounds) = build_configuration(structure.configuration(), composition);
        // build_structure arranges the structure in forward arrangement (sorted by species)
        // we have to do the same with the configuration
        configuration = rearrange(configuration, m_arrange_forward);
        assert(configuration.size() == m_structure.num_atoms());

        for (auto i = 0; i < m_structure.num_atoms(); i++) assert(m_structure.configuration()[i] == configuration[i]);
        std::tie(m_configuration_packing_indices, m_packed_configuration) = pack_configuration(configuration);
    }


    IterationSettings::IterationSettings(
            Structure structure,
            composition_t composition,
            const_array_3d_ref_t target_objective,
            const_array_3d_ref_t parameter_weights,
            pair_shell_weights_t shell_weights,
            rank_t iterations,
            int output_configurations,
            std::vector<int> threads_per_rank,
            double atol,
            double rtol,
            iteration_mode mode) : IterationSettings(
                    structure,
                    composition,
                    target_objective,
                    parameter_weights,
                    array_3d_t(boost::extents[1][1][1]),
                    shell_weights,
                    iterations,
                    output_configurations,
                    default_shell_distances(structure.distance_matrix(), atol, rtol),
                    threads_per_rank,
                    atol,
                    rtol,
                    mode)
            {
        m_parameter_prefactors.resize(boost::extents[num_shells()][num_species()][num_species()]);
        m_parameter_prefactors = compute_prefactors(m_shell_matrix, m_shell_weights, m_packed_configuration);
    }


    [[nodiscard]] std::vector<shell_t> IterationSettings::available_shells() const {
        return m_available_shells;
    }

    [[nodiscard]] std::vector<int> IterationSettings::threads_per_rank() const {
        return m_threads_per_rank;
    }

    const_pair_shell_matrix_ref_t IterationSettings::shell_matrix() const {
        return m_shell_matrix;
    }

    [[nodiscard]] std::vector<AtomPair> IterationSettings::pair_list() const {
        return Structure::create_pair_list(shell_matrix(), m_shell_weights);
    }

    [[nodiscard]] const Structure& IterationSettings::structure() const {
        return m_structure;
    }

    [[nodiscard]] size_t IterationSettings::num_atoms() const {
        return m_structure.num_atoms();
    }

    [[nodiscard]] rank_t IterationSettings::num_iterations() const {
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

    [[nodiscard]] const_array_3d_ref_t IterationSettings::target_objective() const {
        return m_target_objective;
    }

    pair_shell_weights_t IterationSettings::shell_weights() const {
        return m_shell_weights;
    }

    [[nodiscard]] configuration_t IterationSettings::packed_configuraton() const {
        return m_packed_configuration;
    }

    [[nodiscard]] const_array_3d_ref_t IterationSettings::parameter_weights() const {
        return m_parameter_weights;
    }

    iteration_mode IterationSettings::mode() const {
        return m_mode;
    }

    [[nodiscard]] std::tuple<std::vector<shell_t>, std::vector<double>> IterationSettings::shell_indices_and_weights() const {
        return compute_shell_indices_and_weights(m_shell_weights);
    }

    [[nodiscard]] const_array_3d_ref_t IterationSettings::parameter_prefactors() const {
        return m_parameter_prefactors;
    }

    [[nodiscard]] configuration_t IterationSettings::unpack_configuration(const configuration_t &conf) const{
        return utils::unpack_configuration(m_configuration_packing_indices, conf);
    }

    [[nodiscard]] double IterationSettings::atol() const {
        return m_atol;
    }

    [[nodiscard]] composition_t IterationSettings::composition() const {
        return m_composition;
    }

    [[nodiscard]] double IterationSettings::rtol() const {
        return m_rtol;
    }

    [[nodiscard]] arrangement_t IterationSettings::arrange_forward() const {
        return m_arrange_forward;
    }

    [[nodiscard]] arrangement_t IterationSettings::arrange_backward() const {
        return m_arrange_backward;
    }

    [[nodiscard]] shuffling_bounds_t IterationSettings::shuffling_bounds() const {
        return m_shuffling_bounds;
    }
}