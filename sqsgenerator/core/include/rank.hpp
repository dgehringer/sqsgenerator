//
// Created by dominik on 21.05.21.
//

#ifndef SQSGENERATOR_RANK_H
#define SQSGENERATOR_RANK_H

#include "types.hpp"

using namespace boost::multiprecision;

namespace sqsgenerator::utils {

        rank_t total_permutations(const configuration_t &conf);
        rank_t rank_permutation(const configuration_t &conf, size_t nspecies);
        void unrank_permutation(configuration_t &conf, std::vector<size_t> hist, rank_t total_permutations,  rank_t rank);
    }
#endif //SQSGENERATOR_RANK_H
