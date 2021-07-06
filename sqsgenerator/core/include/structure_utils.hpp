//
// Created by dominik on 25.06.21.
//

#ifndef SQSGENERATOR_STRUCTURE_UTILS_H
#define SQSGENERATOR_STRUCTURE_UTILS_H

#include "utils.hpp"
#include "types.hpp"
#include <vector>
#include <assert.h>
#include <limits>
#include <algorithm>
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>

using namespace boost;
using namespace boost::numeric::ublas;


namespace sqsgenerator::utils {

        template<typename T>
        multi_array<T, 3> pbc_shortest_vectors(const matrix<T> &lattice, const matrix<T> &coords, bool frac_coords = false){
            const matrix<T> cart_coords(frac_coords ? prod(coords, lattice): coords);
            auto num_atoms {coords.size1()};
            auto a {row(lattice, 0)};
            auto b {row(lattice, 1)};
            auto c {row(lattice, 2)};

            std::vector<int> axis {-1, 0, 1};
            multi_array<T, 3> vecs(boost::extents[num_atoms][num_atoms][3]);
            // pi1 = position_index_1
            // pi2 = position_index_2
            for (size_t pi1 = 0; pi1 < num_atoms; pi1++) {
                auto p1 {row(cart_coords, pi1)};
                for (size_t pi2 = pi1+1; pi2 < num_atoms; pi2++) {
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
                                    for (size_t dim = 0; dim < 3; dim++) {
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

        template<typename T>
        multi_array<T, 2> distance_matrix(const multi_array<T, 3> &vecs){
            std::vector<size_t> shape(shape_from_multi_array<T,3>(vecs));
            size_t num_atoms = shape[0];
            multi_array<T, 2> d2(boost::extents[num_atoms][num_atoms]);
            for (size_t i = 0; i < num_atoms; i++) {
                for (size_t j = i; j < num_atoms; j++) {
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

        template<typename T>
        multi_array<T, 2> distance_matrix(const matrix<T> &lattice, const matrix<T> &coords, bool frac_coords = false){
            return distance_matrix(pbc_shortest_vectors(lattice, coords, frac_coords));
        }



        template<typename T>
        multi_array<int, 2> shell_matrix(const multi_array<T, 2> &distance_matrix, uint8_t prec = 5) {
            std::vector<size_t> shape(shape_from_multi_array<T,2>(distance_matrix));
            size_t num_atoms = shape[0];
            multi_array<T, 2> rounded(boost::extents[num_atoms][num_atoms]);
            multi_array<int, 2> shells(boost::extents[num_atoms][num_atoms]);

            for (size_t i = 0; i < num_atoms; i++) {
                for (size_t j = i; j < num_atoms; j++) {
                    T rounded_distance = round_nplaces(distance_matrix[i][j], prec);
                    rounded[i][j] = rounded_distance;
                    rounded[j][i] = rounded_distance;
                }
            }

            std::vector<T> unique (rounded.data(), rounded.data() + rounded.num_elements());
            std::sort( unique.begin(), unique.end() );
            unique.erase( std::unique( unique.begin(), unique.end() ), unique.end() );

            for (size_t i = 0; i < num_atoms; i++) {
                for (size_t j = i; j < num_atoms; j++) {
                    int shell {get_index(unique, rounded[i][j])};
                    if (shell < 0) throw std::runtime_error("A shell was detected which I am not aware of");
                    shells[i][j] = shell;
                    shells[j][i] = shell;
                }
            }

            return shells;
        }

    }

#endif //SQSGENERATOR_STRUCTURE_UTILS_H
