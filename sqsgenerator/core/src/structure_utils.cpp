//
// Created by dominik on 19.07.21.
//

#include "structure_utils.hpp"

namespace sqsgenerator::utils {

    std::map<shell_t, pair_shell_matrix_t::index> shell_index_map(const pair_shell_weights_t &weights) {
        typedef pair_shell_matrix_t::index index_t;
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

}