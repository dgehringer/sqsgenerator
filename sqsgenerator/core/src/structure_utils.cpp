//
// Created by dominik on 19.07.21.
//

#include "structure_utils.hpp"

namespace sqsgenerator::utils {

    std::map<shell_t, index_t> shell_index_map(const pair_shell_weights_t &weights) {
        index_t nshells{static_cast<index_t>(weights.size())};
        std::vector<shell_t> shells;
        std::map<shell_t, index_t> shell_indices;
        // Copy the shells into a new vector
        for (const auto &shell : weights) shells.push_back(shell.first);
        // We sort the shells so that they are in ascending order
        std::sort(shells.begin(), shells.end());
        // Create the shell-index map, using a simple enumeration
        for (index_t i = 0; i < nshells; i++) shell_indices.emplace(std::make_pair(shells[i], i));
        return shell_indices;
    }

    std::vector<AtomPair>
    create_pair_list(const pair_shell_matrix_t &shell_matrix, const std::map<shell_t, double> &weights) {
        typedef pair_shell_matrix_t::index index_t;
        std::vector<AtomPair> pair_list;
        auto shell_indices(shell_index_map(weights));
        auto shape = shape_from_multi_array(shell_matrix);
        auto[sx, sy] = std::make_tuple(static_cast<index_t>(shape[0]), static_cast<index_t>(shape[1]));
        assert(shape.size() == 2);
        for (index_t i = 0; i < sx; i++) {
            for (index_t j = i + 1; j < sy; j++) {
                shell_t shell = shell_matrix[i][j];
                if (shell_indices.find(shell) != shell_indices.end())
                    pair_list.push_back(AtomPair{i, j, shell, shell_indices[shell]});
            }
        }
        return pair_list;
    }

    std::tuple<std::vector<shell_t>, std::vector<double>> compute_shell_indices_and_weights(const pair_shell_weights_t &shell_weights) {
        std::vector<shell_t> shells;
        std::vector<double> sorted_shell_weights;
        for (const auto &weight: shell_weights) shells.push_back(weight.first);
        std::sort(shells.begin(), shells.end());
        for (const auto &shell: shells) sorted_shell_weights.push_back(shell_weights.at(shell));
        return std::make_tuple(shells, sorted_shell_weights);
    }

    array_3d_t compute_prefactors(const_pair_shell_matrix_ref_t shell_matrix, const pair_shell_weights_t &shell_weights, const configuration_t &configuration) {
        auto nshells {static_cast<index_t>(shell_weights.size())};
        auto nspecies {static_cast<index_t>(unique_species(configuration).size())};
        auto natoms {static_cast<index_t>(configuration.size())};
        auto natoms_d {static_cast<double>(configuration.size())};

        std::map<shell_t, double> neighbor_count;
        std::vector<shell_t> shells = std::get<0>(compute_shell_indices_and_weights(shell_weights));
        for (const auto &shell: shells) neighbor_count.emplace(shell, 0.0);

        for (auto i = 0; i < natoms; i++) {
            for (auto j = i + 1; j < natoms; j++) {
                auto neighbor_shell {shell_matrix[i][j]};
                if (neighbor_count.count(neighbor_shell)) {
                    assert(neighbor_shell == shell_matrix[j][i]);
                    neighbor_count[neighbor_shell] += 2.0;
                }
            }
        }

        for (const auto& [shell_index, num_neighbors] : neighbor_count) neighbor_count[shell_index] /= natoms_d;

        BOOST_LOG_TRIVIAL(info) << "structure_utils::compute_prefactors::shells = " << format_vector(shells);
        BOOST_LOG_TRIVIAL(info) << "structure_utils::compute_prefactors::shell_weights = " << format_map(shell_weights);
        BOOST_LOG_TRIVIAL(info) << "structure_utils::compute_prefactors::neighbor_count = " << format_map(neighbor_count);

        auto sum_neighbors {0};
        for (const auto&[shell_index, shell_atoms] : neighbor_count) {
            if (shell_atoms < 1) BOOST_LOG_TRIVIAL(warning) <<"The coordination shell " + std::to_string(shell_index) + R"( contains no or only one lattice position(s) increase either "atol" or "rtol" or to set the "shell_distances" parameter manually)";

            auto is_integer { is_close(shell_atoms, static_cast<double>(static_cast<shell_t>(shell_atoms))) };
            if (!is_integer) {
                BOOST_LOG_TRIVIAL(warning) << "structure_utils::compute_prefactors::shell::" << shell_index << "::no_integer_coordination_shell = " << shell_atoms;
            }
            else {
                BOOST_LOG_TRIVIAL(info) << "structure_utils::compute_prefactors::shell::" << shell_index << "::is_integer_coordination_shell = " << shell_atoms;
            }
            sum_neighbors += shell_atoms;
        }
        BOOST_LOG_TRIVIAL(debug) << "structure_utils::compute_prefactors::sum_neighbors = " << sum_neighbors;

        auto hist = configuration_histogram(configuration);
        array_3d_t parameter_prefactors(boost::extents[static_cast<index_t>(nshells)][static_cast<index_t>(nspecies)][static_cast<index_t>(nspecies)]);

        for (index_t i = 0; i < nshells; i++) {
            double M_i {static_cast<double>(neighbor_count[shells[i]])};
            for (index_t a = 0; a < nspecies; a++) {
                double x_a {static_cast<double>(hist[a])/natoms_d};
                for (index_t b = a; b < nspecies; b++) {
                    double x_b {static_cast<double>(hist[b])/natoms_d};
                    double prefactor {1.0/(M_i*x_a*x_b*natoms_d)};
                    parameter_prefactors[i][a][b] = prefactor;
                    parameter_prefactors[i][b][a] = prefactor;
                }
            }
        }
        return parameter_prefactors;
    }

}