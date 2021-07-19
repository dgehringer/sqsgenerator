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
        std::vector<typename MultiArray::element> default_shell_distances(const MultiArray &distance_matrix, typename MultiArray::element atol = 1.0e-5, typename MultiArray::element rtol=1.0e-8) {
            typedef typename MultiArray::index index_t;
            typedef typename MultiArray::element T;
            auto shape(shape_from_multi_array(distance_matrix));
            auto num_atoms {static_cast<index_t>(shape[0])};
            multi_array<T, 2> rounded(boost::extents[num_atoms][num_atoms]);
            std::map<T, std::vector<T>> shell_dists;

            auto is_close_tol = [=] (T a, T b) {
                return is_close(a, b, atol, rtol);
            };

            std::function<std::tuple<bool, T>(T)> is_shell_border_nearby = [&] (T distance) {
                for (const auto &found_dist : shell_dists) {if (is_close_tol(distance, found_dist.first)) return std::make_tuple(true, found_dist.first); }
                return std::make_tuple(false, -1.0);
            };


            for (index_t i = 0; i < num_atoms; i++) {
                for (index_t j = i; j < num_atoms; j++) {
                    T distance {distance_matrix[i][j]};
                    auto [has_shell_nearby, shell_dist] = is_shell_border_nearby(distance);
                    if (has_shell_nearby) shell_dists[shell_dist].push_back(distance);
                    else shell_dists[distance].push_back(distance);
                }
            }

            std::vector<T> result;
            for (const auto &pair : shell_dists) result.push_back(*std::max_element(pair.second.begin(), pair.second.end()));
            std::sort(result.begin(), result.end());

            BOOST_LOG_TRIVIAL(info) << "structure_utils::default_shell_distances::num_distances  = " + std::to_string(result.size());
            BOOST_LOG_TRIVIAL(info) << "structure_utils::default_shell_distances::distances  = " + format_vector(result);

            return result;
        }

        template<typename MultiArray>
        pair_shell_matrix shell_matrix(const MultiArray &distance_matrix, const std::vector<typename MultiArray::element> &distances, typename MultiArray::element atol = 1.0e-5, typename MultiArray::element rtol=1.0e-8) {

            typedef typename MultiArray::index index_t;
            typedef typename MultiArray::element T;
            auto shape(shape_from_multi_array(distance_matrix));
            auto num_atoms {static_cast<index_t>(shape[0])};
            pair_shell_matrix shells(boost::extents[num_atoms][num_atoms]);

            auto is_close_tol = [&atol, &rtol] (T a, T b) {
                return is_close(a, b, atol, rtol);
            };

            auto find_shell = [&distances, &is_close_tol] (T distance) {
                int shell {-1};
                if (is_close_tol(distance, 0.0)) shell = 0;
                else {
                    for (size_t i = 0; i < distances.size() -1; i++) {
                        T lower_bound {distances[i]}, upper_bound {distances[i+1]};
                        if ((is_close_tol(distance, lower_bound) or lower_bound < distance) and (is_close_tol(distance, upper_bound) or upper_bound > distance)) {
                            return static_cast<int>(i+1);
                        }
                    }
                }
                return static_cast<int>(distances.size());
            };

            for (index_t i = 0; i < num_atoms; i++) {
                for (index_t j = i + 1; j < num_atoms; j++) {
                    int shell {find_shell(distance_matrix[i][j])};
                    //std::cout << "(" << i << ", " << j << ", d = " << distance_matrix[i][j] <<") = " << shell << std::endl;
                    if (shell < 0) throw std::runtime_error("A shell was detected which I am not aware of");
                    else if (shell == 0 and i != j) {
                        BOOST_LOG_TRIVIAL(warning) << "Atoms " + std::to_string(i) + " and " + std::to_string(j) + " are overlapping! (distance = " + std::to_string(distance_matrix[i][j])<< ", shell = " << shell <<")!";
                    }
                    shells[i][j] = static_cast<shell_t>(shell);
                    shells[j][i] = static_cast<shell_t>(shell);
                }
            }
            return shells;
        }

        std::map<shell_t, pair_shell_matrix::index> shell_index_map(const pair_shell_weights_t &weights);

        std::vector<AtomPair> create_pair_list(const pair_shell_matrix &shell_matrix, const std::map<shell_t, double> &weights);

    }

#endif //SQSGENERATOR_STRUCTURE_UTILS_HPP
