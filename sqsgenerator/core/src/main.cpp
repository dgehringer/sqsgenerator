//
// Created by dominik on 21.05.21.
#include "sqs.hpp"
#include "utils.hpp"
#include "rank.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include "types.hpp"
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>


using namespace sqsgenerator;
using namespace sqsgenerator::utils;
using namespace boost::numeric::ublas;

void print_conf(configuration_t conf, bool endl = true) {
    std::cout << "{";
    for (int i = 0; i < conf.size()-1; ++i) {
        std::cout << static_cast<int>(conf[i]) << ", ";
    }
    std::cout << static_cast<int>(conf.back()) << "}";
    if (endl) std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    (void) argc, (void) argv;
    using namespace sqsgenerator;


    size_t nshells{7}, nspecies{2};
    /* std::vector<double> data{0, 1, 1, 0, 0, 2, 2, 0, 0, 3, 3, 0, 0, 4, 4, 0, 0, 5, 5, 0, 0, 6, 6, 0, 0, 7, 7, 0};

    configuration_t conf{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};
    SQSResult result(0.0, 1, conf, data);
    matrix<double, row_major, std::vector<double>> A(7, 4, data);
    auto  array = result.parameters<2>(Shape<2>{7,4});
    print_array(array, nshells, nspecies*nspecies);
    SQSResultCollection results(5);
    size_t nthread = 8;


    std::thread generator([&]() {
        for (size_t i = 1; i < 11; i++) {
            double objective {1.0 / (double)i};
            uint64_t rank = i;
            std::cout << " --------- LOOP ----------" << std::endl;
            SQSResult result {objective, i, conf, data};
            //result.rank = rank;
            //result.objective = objective;
            std::cout << " --------- ADDING----------" << std::endl;
            bool res = results.add_result(result);

            std::cout << i << ": " << results.best_objective() << ", " << objective << std::endl;
        }
        //results.addResult(result);
        //results.addResult(result);
    });


    generator.join();
    results.collect();
    assert(results.best_objective() == 1.0 / 10.0);
    std::cout << results.best_objective() << ", " << results.result_size() << std::endl;
    std::cout << "==============================" << std::endl;

    multi_array<double, 3> sro_params(boost::extents[nshells][nspecies][nspecies]);
    sro_params.assign(data.begin(), data.end());

    multi_array_ref<double, 3> sro_params_ref(sro_params.data(), boost::extents[nshells][nspecies][nspecies]);

    std::fill(sro_params_ref.data(), sro_params_ref.data() + sro_params_ref.num_elements(), 1);
    std::cout << sro_params_ref[0][0][0] << "\n";*/


    array_2d_t lattice = boost::make_multi_array<double, 3, 3>({
        8.10, 0.00, 0.00,
        0.00, 8.10, 0.00,
        0.00, 0.00, 8.10
    });

    array_2d_t frac_coords = boost::make_multi_array<double, 16, 3>({
           0.  , 0.  , 0.,
           0.25, 0.25, 0.25,
           0.  , 0.  , 0.5,
           0.25, 0.25, 0.75,
           0.  , 0.5 , 0.,
           0.25, 0.75, 0.25,
           0.  , 0.5 , 0.5,
           0.25, 0.75, 0.75,
           0.5 , 0.  , 0.,
           0.75, 0.25, 0.25,
           0.5 , 0.  , 0.5,
           0.75, 0.25, 0.75,
           0.5 , 0.5 , 0.,
           0.75, 0.75, 0.25,
           0.5 , 0.5 , 0.5,
           0.75, 0.75, 0.75
    });

    std::vector<std::string> species { "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Cs", "Cl" };
    /*std::vector<std::string> species { "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Ne", "Ne", "Ne", "Ne", "Ne", "Ne", "Ne", "Ne" };*/
    std::array<bool, 3> pbc {true, true, true};

    Structure structure(lattice, frac_coords, species, pbc);

    pair_shell_weights_t shell_weights {
            {4, 0.25},
            {1, 1.0},
            {3, 0.33},
            {2, 0.5}
    };

    array_2d_t pair_weights = boost::make_multi_array<double, 2, 2>({
        1.0, 1.0, 1.0, 1.0
    });

    /* array_2d_t pair_weights = boost::make_multi_array<double, 4, 4>({
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0
    });*/
    auto conf (structure.configuration());


    IterationSettings settings(structure, 0.0, pair_weights, shell_weights, 1000, 10, iteration_mode::random);
    auto initial_rank = rank_permutation(settings.packed_configuraton(), settings.num_species());
    std::cout << "[MAIN]: " << structure.num_atoms() << " - " << conf.size() << std::endl;
    std::cout << "[MAIN]: rank = " << initial_rank  << std::endl;
    std::cout << "[MAIN]: configuration = "; print_conf(structure.configuration());
    std::cout << "[MAIN]: packed_config = "; print_conf(settings.packed_configuraton());
    std::cout << "[MAIN]: num_pairs = " << settings.pair_list().size() << std::endl;
    do_iterations(settings);
}
//

