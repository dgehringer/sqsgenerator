//
// Created by dominik on 30.06.21.
//


#include "sqs.hpp"
#include "types.hpp"
#include <set>
#include <boost/multi_array.hpp>


using namespace boost;


template<>
void sqsgenerator::count_pairs<>(const Configuration &configuration, const std::set<int> &shells, multi_array_ref<double, 3> &bonds, multi_array_ref<int, 2> &shells_matrix){
    std::cout << "Count pairs\n";
    Species first_species, second_species;
    int pair_shell {-1};
    for (size_t i = 0; i < configuration.size() ; i++) {
        first_species = configuration[i];
        for (size_t j = i + 1; j < configuration.size() ; j++) {
            pair_shell = shells_matrix[i][j];
            if (pair_shell < 0) continue;
            second_species = configuration[j];


        }
    }
}


