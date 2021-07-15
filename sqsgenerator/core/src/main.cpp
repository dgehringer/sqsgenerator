//
// Created by dominik on 21.05.21.
#include "sqs.hpp"
#include "utils.hpp"
#include "rank.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "types.hpp"
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>


using namespace sqsgenerator;
using namespace sqsgenerator::utils;
using namespace boost::numeric::ublas;

void print_conf(configuration_t conf, bool endl = true) {
    std::cout << "{";
    for (size_t i = 0; i < conf.size()-1; ++i) {
        std::cout << static_cast<int>(conf[i]) << ", ";
    }
    std::cout << static_cast<int>(conf.back()) << "}";
    if (endl) std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    (void) argc, (void) argv;
    using namespace sqsgenerator;


    size_t nspecies{2};

    array_2d_t lattice = boost::make_multi_array<double, 3, 3>({
           0.0000,6.0750,6.0750,
           6.0750,0.0000,6.0750,
           6.0750,6.0750,0.0000
    });



    array_2d_t frac_coords = boost::make_multi_array<double, 27, 3>({
            0.000000,0.000000,0.000000,
            0.000000,0.000000,0.000000,
            0.000000,0.000000,0.000000,
            0.333333,0.333333,0.333333,
            0.333333,0.333333,0.333333,
            0.333333,0.333333,0.333333,
            0.666667,0.666667,0.666667,
            0.666667,0.666667,0.666667,
            0.666667,0.666667,0.666667,
            0.000000,0.000000,0.000000,
            0.333333,0.333333,0.333333,
            0.666667,0.666667,0.666667,
            0.000000,0.000000,0.000000,
            0.333333,0.333333,0.333333,
            0.666667,0.666667,0.666667,
            0.000000,0.000000,0.000000,
            0.333333,0.333333,0.333333,
            0.666667,0.666667,0.666667,
            0.000000,0.333333,0.666667,
            0.000000,0.333333,0.666667,
            0.000000,0.333333,0.666667,
            0.000000,0.333333,0.666667,
            0.000000,0.333333,0.666667,
            0.000000,0.333333,0.666667,
            0.000000,0.333333,0.666667,
            0.000000,0.333333,0.666667,
            0.000000,0.333333,0.666667
    });

    std::vector<std::string> species { "Al","Al","Al","Al","Al","Al","Al","Al","Al","Al","Al","Al","Al","Ni","Ni","Ni","Ni","Ni","Ni","Ni","Ni","Ni","Ni","Ni","Ni","Ni","Ni" };
    /*std::vector<std::string> species { "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Ne", "Ne", "Ne", "Ne", "Ne", "Ne", "Ne", "Ne" };*/
    std::array<bool, 3> pbc {true, true, true};

    Structure structure(lattice, frac_coords, species, pbc);
    structure.shell_matrix(2);

    pair_shell_weights_t shell_weights {
           /*{4, 0.25},
           {5, 0.2},*/
            {1, 1.0},
           {3, 0.33},
           {2, 0.5},
            {6, 1.0/6.0},
            {7, 1.0/7.0}
    };

    array_2d_t pair_weights = boost::make_multi_array<double, 2, 2>({
        1.0, 2.0, 5.0, 2.0
    });

    std::cout << "max_element = " << *std::max_element(pair_weights.data(), pair_weights.data()+4) << std::endl;
    /* array_2d_t pair_weights = boost::make_multi_array<double, 4, 4>({
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0
    });*/
    auto conf (structure.configuration());

    array_3d_t target_objective(boost::extents[shell_weights.size()][nspecies][nspecies]);
    //std::fill(target_objective.begin(), target_objective.end(), 0.0);

    auto niteration {10};
    omp_set_num_threads(1);
    IterationSettings settings(structure, target_objective, pair_weights, shell_weights, niteration, 10, iteration_mode::random);
    auto initial_rank = rank_permutation(settings.packed_configuraton(), settings.num_species());
    std::cout << "[MAIN]: " << structure.num_atoms() << " - " << conf.size() << std::endl;
    std::cout << "[MAIN]: rank = " << initial_rank  << std::endl;
    std::cout << "[MAIN]: configuration = "; print_conf(structure.configuration());
    std::cout << "[MAIN]: packed_config = "; print_conf(settings.packed_configuraton());
    std::cout << "[MAIN]: num_pairs = " << settings.pair_list().size() << std::endl;
    std::cout << "[MAIN]: num_threads = " << omp_get_num_threads() << std::endl;
    std::cout << "[MAIN]: num_max_threads = " << omp_get_max_threads() << std::endl;
    std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
    do_iterations_vector(settings);
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::cout << "Time difference = " << static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count())/niteration << " [µs]" << std::endl;
}


