//
// Created by dominik on 30.06.21.
//

#ifndef SQSGENERATOR_SQS_HPP
#define SQSGENERATOR_SQS_HPP


#include "types.hpp"
#include "rank.hpp"
#include "utils.hpp"
#include "atomistics.hpp"
#include "containers.hpp"
#include <map>
#include <limits>
#include <omp.h>
#include <vector>
#include <cassert>
#include <algorithm>

using namespace sqsgenerator::utils;
using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator {

    template<typename MultiArray>
    void count_pairs(const configuration_t &configuration, const std::vector<AtomPair> &pair_list, MultiArray &bonds, bool clear = false){
        typedef typename MultiArray::index index_t;
        typedef typename MultiArray::element T;
        std::vector<index_t> shape {shape_from_multi_array(bonds)};
        assert(shape.size() ==3);
        // the array must have the following dimensions (nshells, nspecies, nspecies)
        // configuration is already the remapped configuration {H, H, He, He, Ne, Ne} -> {0, 0, 1, 1, 2, 2}
        T increment {static_cast<T>(1.0)};
        index_t nshells{shape[0]}, nspecies{shape[1]};
        species_t si, sj;
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

    double calculate_objective();

    void next_permutation(configuration_t &conf) {

    }

    void do_iterations(const Structure &structure, int niterations, double objective, int noutput, const std::map<shell_t, double> &weights) {

        double best_objective {std::numeric_limits<double>::max()};
        double best_objective_local {best_objective};
        configuration_t configuration_local;

        std::vector<AtomPair> pair_list {structure.create_pair_list(weights)};
        std::map<rank_t, SQSResult> results;
        # ifdef _OPENMP
        #pragma omp parallel for default(none) shared(best_objective, results) firstprivate(niterations, best_objective_local, configuration_local, pair_list)
        # endif
        for (size_t i = 0; i < niterations; i++) {
            next_permutation(configuration_local);
        }

    }
}

#endif //SQSGENERATOR_SQS_HPP
