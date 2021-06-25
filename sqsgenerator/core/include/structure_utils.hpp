//
// Created by dominik on 25.06.21.
//

#ifndef SQSGENERATOR_STRUCTURE_UTILS_H
#define SQSGENERATOR_STRUCTURE_UTILS_H

#include "utils.hpp"
#include "types.hpp"
#include <vector>
#include <assert.h>
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
        void calculate_distance_matrix(const matrix<T> &lattice, const matrix<T> &coords,  bool frac_coords = false){
            const matrix<T> cart_coords(frac_coords ? prod(coords, lattice): coords);
            auto a {row(lattice, 0)};
            auto b {row(lattice, 1)};
            auto c {row(lattice, 2)};


            auto t = a+b+c;
            std::cout << "[";
            for (size_t i = 0; i < t.size()-1; i++) {

                std::cout << t(i) << ", ";
            }
            std::cout<< t(t.size()-1) << "]" << std::endl;
        }

        template<typename T>
        void calculate_distance_matrix(const std::vector<T> &lattice, const std::vector<T> &coords,  bool frac_coords = false){
            assert(lattice.size() == 9);
            assert(coords.size() % 3 == 0);
            calculate_distance_matrix(matrix_from_vector(3, 3, lattice), matrix_from_vector(coords.size() /3, 3, coords), frac_coords);
        }



    }
}

#endif //SQSGENERATOR_STRUCTURE_UTILS_H
