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
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>

using namespace boost;
using namespace boost::numeric::ublas;

template<typename T>
void print_matrix(const matrix<T> &mat) {
    std::cout << "[";
    for (size_t i = 0; i < mat.size1(); i++) {
        std::cout << "[";
        for (size_t j = 0; j < mat.size2() - 1; j++) {
            std::cout << mat(i, j) << ", ";
        }
        std::cout << mat(i, mat.size2() - 1) << "]";
        if (i < mat.size1() - 1) std::cout << std::endl;
    }
    std::cout << "]" << std::endl;
}

template<typename T>
void print_matrix_row(const matrix_row<matrix<T>> &mat) {
    std::cout << "[";
    for (size_t i = 0; i < mat.size()-1; i++) {

            std::cout << mat(i) << ", ";
    }
    std::cout<< mat(mat.size()-1) << "]" << std::endl;
}

namespace sqsgenerator {
    namespace utils {

        template<typename T>
        multi_array<T, 3> calculate_distance_matrix(const matrix<T> &lattice, const matrix<T> &coords,  bool frac_coords = false){
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
                                        vecs[pi2][pi1][dim] = diff(dim);
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
        multi_array<T, 3> calculate_distance_matrix(const std::vector<T> &lattice, const std::vector<T> &coords,  bool frac_coords = false){
            assert(lattice.size() == 9);
            assert(coords.size() % 3 == 0);
            return calculate_distance_matrix(matrix_from_vector(3, 3, lattice), matrix_from_vector(coords.size() /3, 3, coords), frac_coords);
        }



    }
}

#endif //SQSGENERATOR_STRUCTURE_UTILS_H
