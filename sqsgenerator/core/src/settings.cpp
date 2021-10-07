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
            pair_shell_weights_t shell_weights,
            int iterations,
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
        m_parameter_prefactors(boost::extents[static_cast<index_t>(shell_weights.size())][static_cast<index_t>(m_nspecies)][static_cast<index_t>(m_nspecies)]),
        m_shell_distances(shell_distances),
        m_threads_per_rank(threads_per_rank),
        m_shell_matrix(m_structure.shell_matrix(m_shell_distances, m_atol, m_rtol))
    {
        auto shell_m(shell_matrix());
        auto num_elements {num_atoms() * num_atoms()};
        std::set<shell_t> unique(shell_m.data(), shell_m.data() + num_elements);
        m_available_shells = std::vector<shell_t>(unique.begin(), unique.end());
        std::sort(m_available_shells.begin(), m_available_shells.end());
        m_available_shells.erase(m_available_shells.begin());
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
        init_prefactors();
    }

    IterationSettings::IterationSettings(
            Structure structure,
            composition_t composition,
            const_array_3d_ref_t target_objective,
            const_array_3d_ref_t parameter_weights,
            pair_shell_weights_t shell_weights,
            int iterations,
            int output_configurations,
            std::vector<int> threads_per_rank,
            double atol,
            double rtol,
            iteration_mode mode) :
            IterationSettings(
                    structure,
                    composition,
                    target_objective,
                    parameter_weights,
                    shell_weights,
                    iterations,
                    output_configurations,
                    default_shell_distances(structure.distance_matrix(), atol, rtol),
                    threads_per_rank,
                    atol,
                    rtol,
                    mode)
    { }


    [[nodiscard]] std::vector<shell_t> IterationSettings::available_shells() const {
        return m_available_shells;
    }

    [[nodiscard]] std::vector<int> IterationSettings::threads_per_rank() const {
        return m_threads_per_rank;
    }

    void IterationSettings::init_prefactors() {
        auto nshells {static_cast<index_t>(num_shells())};
        auto nspecies {static_cast<index_t>(num_species())};
        auto natoms {static_cast<index_t>(num_atoms())};
        auto natoms_d {static_cast<double>(num_atoms())};

        std::vector<shell_t> shells = std::get<0>(shell_indices_and_weights());
        std::map<shell_t, size_t> neighbor_count;
        for (const auto &shell: shells) neighbor_count.emplace(std::make_pair(shell, 0));

        auto shell_mat = shell_matrix();
        for (auto i = 1; i < natoms; i++) {
            auto neighbor_shell {shell_mat[0][i]};
            if (neighbor_count.count(neighbor_shell)) neighbor_count[neighbor_shell]++;
        }

        BOOST_LOG_TRIVIAL(warning) << "shell_matrix";
        for (auto i = 0; i < natoms; i++) {
            std::map<shell_t, size_t> site_histogram;
            for (const auto &shell: shells) site_histogram.emplace(std::make_pair(shell, 0));

            for (auto j = 0; j < natoms; j++) {
                auto neighbor_shell {shell_mat[i][j]};
                if (site_histogram.count(neighbor_shell)) site_histogram[neighbor_shell]++;
            }
            std::vector<shell_t> keys;
            std::vector<size_t> values;
            for (const auto &[k, v] : site_histogram) {
                keys.push_back(k);
                values.push_back(v);
            }
            BOOST_LOG_TRIVIAL(warning) << format_dict(keys, values);
        }

        auto sum_neighbors {0};
        for (const auto&[shell_index, shell_atoms] : neighbor_count) {
            BOOST_LOG_TRIVIAL(debug) << "IterationSettings::ctor::init_prefactors::shell::" << shell_index << "::radius = " << m_shell_distances[shell_index];
            BOOST_LOG_TRIVIAL(debug) << "IterationSettings::ctor::init_prefactors::shell::" << shell_index << "::num_atoms = " << shell_atoms;
            if (shell_atoms < 2) BOOST_LOG_TRIVIAL(warning) <<"The coordination shell " + std::to_string(shell_index) + R"( contains no or only one lattice position(s) increase either "atol" or "rtol" or to set the "shell_distances" parameter manually)";
            if (shell_atoms < 1) throw std::invalid_argument("The coordination shell " + std::to_string(shell_index) + R"( contains no lattice positions. Please increase either "atol" or "rtol" or set the "shell_distances" parameter manually)");

            sum_neighbors += shell_atoms;
        }
        BOOST_LOG_TRIVIAL(trace) << "IterationSettings::ctor::init_prefactors::sum_neighbors = " << sum_neighbors;

        auto hist = configuration_histogram(m_packed_configuration);
        for (index_t i = 0; i < nshells; i++) {
            double M_i {static_cast<double>(neighbor_count[shells[i]])};
            for (index_t a = 0; a < nspecies; a++) {
                double x_a {static_cast<double>(hist[a])/natoms_d};
                for (index_t b = a; b < nspecies; b++) {
                    double x_b {static_cast<double>(hist[b])/natoms_d};
                    double prefactor {1.0/(M_i*x_a*x_b*natoms_d)};
                    m_parameter_prefactors[i][a][b] = prefactor;
                    m_parameter_prefactors[i][b][a] = prefactor;
                }
            }
        }
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
        std::vector<shell_t> shells;
        std::vector<double> sorted_shell_weights;
        for (const auto &weight: m_shell_weights) shells.push_back(weight.first);
        std::sort(shells.begin(), shells.end());
        for (const auto &shell: shells) sorted_shell_weights.push_back(m_shell_weights.at(shell));
        return std::make_tuple(shells, sorted_shell_weights);
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