//
// Created by dominik on 25.06.21.
//

#ifndef SQSGENERATOR_STRUCTURE_UTILS_HPP
#define SQSGENERATOR_STRUCTURE_UTILS_HPP


#include "types.hpp"
#include "utils.hpp"
#include <vector>
#include <limits>
#include <algorithm>
#include <boost/multi_array.hpp>
#include <boost/log/trivial.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>

using namespace boost;
using namespace boost::numeric::ublas;


namespace sqsgenerator::utils {

        template<typename T>
        multi_array<T, 3> pbc_shortest_vectors(const matrix<T> &lattice, const matrix<T> &coords, bool frac_coords = false){
            const matrix<T> cart_coords(frac_coords ? prod(coords, lattice): coords);
            auto num_atoms {static_cast<index_t>(coords.size1())};
            auto a {row(lattice, 0)};
            auto b {row(lattice, 1)};
            auto c {row(lattice, 2)};

            std::vector<int> axis {-1, 0, 1};
            multi_array<T, 3> vecs(boost::extents[num_atoms][num_atoms][3]);
            // pi1 = position_index_1
            // pi2 = position_index_2
            for (index_t pi1 = 0; pi1 < num_atoms; pi1++) {
                auto p1 {row(cart_coords, pi1)};
                for (index_t pi2 = pi1+1; pi2 < num_atoms; pi2++) {
                    auto p2 {row(cart_coords, pi2)};
                    T norm {std::numeric_limits<T>::max()};
                    for (auto &i : axis) {
                        for (auto &j: axis) {
                            for (auto &k: axis) {
                                auto t = i * a + j * b + k * c;
                                auto diff = p1 - (t + p2);
                                T image_norm {norm_2(diff)};
                                if (image_norm < norm) {
                                    norm = image_norm;
                                    for (index_t dim = 0; dim < 3; dim++) {
                                        vecs[pi1][pi2][dim] = diff(dim);
                                        vecs[pi2][pi1][dim] = -diff(dim);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return vecs;
        }

        template<typename T>
        multi_array<T, 3> pbc_shortest_vectors(const std::vector<T> &lattice, const std::vector<T> &coords,  bool frac_coords = false){
            assert(lattice.size() == 9);
            assert(coords.size() % 3 == 0);
            return pbc_shortest_vectors(matrix_from_vector(3, 3, lattice),
                                        matrix_from_vector(coords.size() / 3, 3, coords), frac_coords);
        }

        template<typename MultiArray>
        multi_array<typename MultiArray::element, 2> distance_matrix(const MultiArray &vecs){
            typedef typename MultiArray::index index_t;
            typedef typename MultiArray::element T;
            auto shape(shape_from_multi_array(vecs));
            auto num_atoms {static_cast<index_t>(shape[0])};
            multi_array<T, 2> d2(boost::extents[num_atoms][num_atoms]);
            for (index_t i = 0; i < num_atoms; i++) {
                for (index_t j = i; j < num_atoms; j++) {
                    T norm = std::sqrt(
                            vecs[i][j][0]*vecs[i][j][0] +
                            vecs[i][j][1]*vecs[i][j][1] +
                            vecs[i][j][2]*vecs[i][j][2]);
                    d2[i][j] = norm;
                    d2[j][i] = norm;
                }
            }
            return d2;
        }


        template<typename MultiArray>
        pair_shell_matrix_t shell_matrix(const MultiArray &distance_matrix, const std::vector<typename MultiArray::element> &distances, typename MultiArray::element atol = 1.0e-5, typename MultiArray::element rtol=1.0e-8) {

            typedef typename MultiArray::index index_t;
            typedef typename MultiArray::element T;
            auto shape(shape_from_multi_array(distance_matrix));
            auto num_atoms {static_cast<index_t>(shape[0])};
            pair_shell_matrix_t shells(boost::extents[num_atoms][num_atoms]);
            auto is_close_tol = [&atol, &rtol] (T a, T b) {
                return is_close(a, b, atol, rtol);
            };

            auto find_shell = [&distances, &is_close_tol] (T distance) {
                if (distance < 0 ) return -1;
                if (is_close_tol(distance, 0.0)) return 0;
                else {
                    for (size_t i = 0; i < distances.size() - 1; i++) {
                        T lower_bound {distances[i]}, upper_bound {distances[i+1]};
                        if ((is_close_tol(distance, lower_bound) or distance > lower_bound) and (is_close_tol(distance, upper_bound) or upper_bound > distance)) {
                            return static_cast<int>(i+1);
                        }
                    }
                }
                return static_cast<int>(distances.size());
            };

            for (index_t i = 0; i < num_atoms; i++) {
                for (index_t j = i + 1; j < num_atoms; j++) {
                    int shell {find_shell(distance_matrix[i][j])};
                    if (shell < 0) throw std::runtime_error("A shell was detected which I am not aware of");
                    else if (shell == 0 and i != j) {
                        BOOST_LOG_TRIVIAL(warning) << "Atoms " + std::to_string(i) + " and " + std::to_string(j) + " are overlapping! (distance = " + std::to_string(distance_matrix[i][j])<< ", shell = " << shell <<")!";
                    }
                    shells[i][j] = shell;
                    shells[j][i] = shell;
                }
            }
            return shells;
        }

    template<typename MultiArray>
    std::vector<typename MultiArray::element> default_shell_distances(const MultiArray &distance_matrix, typename MultiArray::element atol = 1.0e-5, typename MultiArray::element rtol=1.0e-8) {
        typedef typename MultiArray::index index_t;
        typedef typename MultiArray::element T;
        auto shape(shape_from_multi_array(distance_matrix));
        auto num_atoms {static_cast<index_t>(shape[0])};
        std::vector<T> all_distances(distance_matrix.data(), distance_matrix.data() + distance_matrix.num_elements());
        std::sort(all_distances.begin(), all_distances.end());
        std::vector<T> shell_dists;


        auto is_close_tol = [=] (T a, T b) {
            return is_close(a, b, atol, rtol);
        };

        std::function<int(T)> get_shell_index  = [&](T distance){
            for (auto i = 0; i < shell_dists.size(); i++)  if (is_close_tol(distance, shell_dists[i])) return i;
            return -1;
        };

        for (const auto& distance : all_distances) {
            auto shell_dist_index {get_shell_index(distance)};
            // We average the distances
            if (shell_dist_index >= 0) shell_dists[shell_dist_index] = 0.5 * (shell_dists[shell_dist_index] + distance);
            else shell_dists.push_back(distance);
            std::sort(shell_dists.begin(), shell_dists.end());
        }

        // make a sanity check -> we check here that each of the computed coordination shell occurs at least once
        pair_shell_matrix_t current_shell_matrix(shell_matrix(distance_matrix, shell_dists, atol, rtol));
        std::set<shell_t> unique_shells(current_shell_matrix.data(), current_shell_matrix.data() + current_shell_matrix.num_elements());
        if (unique_shells.size() < shell_dists.size()) {
            auto message = "The number of of computed (default) shells does not match the occuring shells in the shell matrix (computed: " + std::to_string(shell_dists.size()) + " !=" + " shell_matrix: " + std::to_string(unique_shells.size()) + ")";
            BOOST_LOG_TRIVIAL(warning) << message;
            throw std::runtime_error(message);
        }
        return shell_dists;
    }

    std::map<shell_t, index_t> shell_index_map(const pair_shell_weights_t &weights);
    std::vector<AtomPair> create_pair_list(const pair_shell_matrix_t &shell_matrix, const std::map<shell_t, double> &weights);
    std::tuple<std::vector<shell_t>, std::vector<double>> compute_shell_indices_and_weights(const pair_shell_weights_t &shell_weights);
    array_3d_t compute_prefactors(const_pair_shell_matrix_ref_t shell_matrix, const pair_shell_weights_t &shell_weights, const configuration_t &configuration);
}

#endif //SQSGENERATOR_STRUCTURE_UTILS_HPP
