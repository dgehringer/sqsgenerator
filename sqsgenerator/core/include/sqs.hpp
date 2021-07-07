//
// Created by dominik on 30.06.21.
//

#ifndef SQSGENERATOR_SQS_HPP
#define SQSGENERATOR_SQS_HPP

#include "types.hpp"
#include "utils.hpp"
#include <vector>
#include <cassert>
#include <algorithm>

namespace sqsgenerator {

    template<typename MultiArray>
    void count_pairs(const Configuration &configuration, const std::vector<AtomPair> &pair_list, MultiArray &bonds, bool clear = false){
        typedef typename MultiArray::index index_t;
        typedef typename MultiArray::element T;
        std::vector<index_t> shape {shape_from_multi_array(bonds)};
        assert(shape.size() ==3);
        // the array must have the following dimensions (nshells, nspecies, nspecies)
        // configuration is already the remapped configuration {H, H, He, He, Ne, Ne} -> {0, 0, 1, 1, 2, 2}
        T increment {static_cast<T>(1.0)};
        index_t nshells{shape[0]}, nspecies{shape[1]};
        Species si, sj;
        // clear the bond count array -> set it to zero
        if (clear) std::fill(bonds.data(), bonds.data() + bonds.num_elements(), 0.0);
        for (const AtomPair &pair : pair_list) {
            auto [i, j, _, shell_index] = pair;
            si = configuration[i];
            sj = configuration[j];
            bonds[shell_index][si][sj] += increment;
            bonds[shell_index][sj][si] += increment;
        }
    }
}

#endif //SQSGENERATOR_SQS_HPP
