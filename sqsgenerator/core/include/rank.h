//
// Created by dominik on 21.05.21.
//

#ifndef SQSGENERATOR_RANK_H
#define SQSGENERATOR_RANK_H

#include "types.h"
#include <boost/multiprecision/cpp_int.hpp>

using namespace boost::multiprecision;

namespace sqsgenerator {

    namespace utils {

        cpp_int total_permutations(const Configuration &conf);
        const Configuration unique_species(Configuration conf);
        std::vector<size_t> configuration_histogram(const Configuration &conf);
        cpp_int rank_permutation(const Configuration &conf, size_t nspecies);
        void unrank_permutation(Configuration &conf, std::vector<size_t> hist, cpp_int total_permutations,  cpp_int rank);
    }

}
#endif //SQSGENERATOR_RANK_H
