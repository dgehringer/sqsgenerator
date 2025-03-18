//
// Created by Dominik Gehringer on 03.03.25.
//

#ifndef OPTIMIZATION_CONFIG_H
#define OPTIMIZATION_CONFIG_H

#include "sqsgen/types.h"
#include "sqsgen/core/structure.h"

namespace sqsgen::core {
  template<class T> struct optimization_config_data {
    vset<usize_t> sites;
    core::structure<T> structure;
    core::structure<T> sorted;
    std::vector<bounds_t<usize_t>> bounds;
    std::vector<usize_t> sort_order;
    configuration_t species_packed;
    index_mapping_t<specie_t, specie_t> species_map;
    index_mapping_t<usize_t, usize_t> shell_map;
    std::vector<atom_pair<usize_t>> pairs;
    cube_t<T> pair_weights;
    cube_t<T> prefactors;
    cube_t<T> target_objective;
    std::vector<T> shell_radii;
    shell_weights_t<T> shell_weights;
  };
}

#endif //OPTIMIZATION_CONFIG_H
