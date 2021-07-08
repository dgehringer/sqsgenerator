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
        auto shape (shape_from_multi_array(bonds));
        assert(shape.size() ==3);
        // the array must have the following dimensions (nshells, nspecies, nspecies)
        // configuration is already the remapped configuration {H, H, He, He, Ne, Ne} -> {0, 0, 1, 1, 2, 2}
        T increment {static_cast<T>(1.0)};
        index_t nshells{static_cast<index_t>(shape[0])}, nspecies{static_cast<index_t>(shape[1])};
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

    template<typename MultiArrayBonds, typename MultiArrayWeights>
    typename MultiArrayBonds::element calculate_pair_objective(MultiArrayBonds &bonds, const std::vector<typename MultiArrayBonds::element> &shell_weights, const MultiArrayWeights &pair_weights, size_t nspecies){
        typedef typename MultiArrayBonds::index index_t;
        typedef typename MultiArrayBonds::element T;
        auto nshells {shell_weights.size()};
        T total_objective {0.0};
        for (index_t i = 0; i < nshells; i++) {
            T shell_weight {shell_weights[i]};
            for (index_t j = 0; j < nspecies; j++) {
                for (index_t k = i+1; k < nspecies; k++) {
                    T pair_weight {pair_weights[j][k]};
                    T pair_sro {shell_weight * (1.0 - bonds[j][k]*pair_weight)};
                    bonds[j][k] = pair_sro;
                    bonds[k][j] = pair_sro;
                    total_objective += pair_sro;
                }
            }
        }
        return total_objective;
    }

    void next_permutation(configuration_t &conf) {

    }

    void do_iterations(const Structure &structure, int niterations, double objective, int noutput, const std::map<shell_t, double> &weights) {
        typedef array_3d_ref_t::index index_t;
        double best_objective {std::numeric_limits<double>::max()};
        double objective_local {best_objective};
        configuration_t configuration_local, unique_species;
        std::tie(unique_species, configuration_local) = structure.remapped_configuration();
        index_t nspecies {static_cast<index_t>(unique_species.size())};
        index_t nshells {static_cast<index_t>(weights.size())};
        index_t nparams {nspecies*nspecies*nshells};
        parameter_storage_t parameters_storage_local(nparams);
        array_3d_ref_t parameters_local(parameters_storage_local.data(), boost::extents[nshells][nspecies][nspecies]);


        std::vector<AtomPair> pair_list {structure.create_pair_list(weights)};
        std::map<rank_t, SQSResult> results;
        #pragma omp parallel default(none) shared(best_objective, results) firstprivate(niterations, objective_local, configuration_local, pair_list, parameters_local)
        {

            #pragma omp for schedule(dynamic)
            for (size_t i = 0; i < niterations; i++) {
                next_permutation(configuration_local);
                count_pairs(configuration_local, pair_list, parameters_local, true);
                // TODO: Implement Settings class

            }

        }

    }
}

#endif //SQSGENERATOR_SQS_HPP
