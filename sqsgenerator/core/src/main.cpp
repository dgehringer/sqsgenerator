//
// Created by dominik on 21.05.21.
#include "sqs.hpp"
#include <mpi.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "types.hpp"
#include <boost/multi_array.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>

namespace logging = boost::log;
using namespace sqsgenerator;
using namespace sqsgenerator::utils;
using namespace boost::numeric::ublas;


static bool log_initialized = false;

void init_logging() {
    if (! log_initialized) {
        static const std::string COMMON_FMT("[%TimeStamp%][%Severity%]: %Message%");
        boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
        boost::log::add_console_log(
                std::clog,
                boost::log::keywords::format = COMMON_FMT,
                boost::log::keywords::auto_flush = true
        );
        boost::log::add_common_attributes();
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::trace);
        log_initialized = true;

    }
}

void print_conf(configuration_t conf, bool endl = true) {
    std::cout << "{";
    for (size_t i = 0; i < conf.size()-1; ++i) {
        std::cout << static_cast<int>(conf[i]) << ", ";
    }
    std::cout << static_cast<int>(conf.back()) << "}";
    if (endl) std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    init_logging();
    (void) argc, (void) argv;
    using namespace sqsgenerator;


    size_t nspecies{2};

    array_2d_t lattice = boost::make_multi_array<double, 3, 3>({
           0.0, 8.1 ,8.1,
           8.1, 0.0, 8.1,
           8.1, 8.1 ,0.0
    });



    array_2d_t frac_coords = boost::make_multi_array<double, 64, 3>({
            0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
            0.00000000e+00, 0.00000000e+00, 2.50000000e-01,
            0.00000000e+00, 0.00000000e+00, 5.00000000e-01,
            0.00000000e+00, 0.00000000e+00, 7.50000000e-01,
            0.00000000e+00, 2.50000000e-01, 0.00000000e+00,
            0.00000000e+00, 2.50000000e-01, 2.50000000e-01,
            0.00000000e+00, 2.50000000e-01, 5.00000000e-01,
            0.00000000e+00, 2.50000000e-01, 7.50000000e-01,
            0.00000000e+00, 5.00000000e-01, 0.00000000e+00,
            5.48258284e-17, 5.00000000e-01, 2.50000000e-01,
            0.00000000e+00, 5.00000000e-01, 5.00000000e-01,
            0.00000000e+00, 5.00000000e-01, 7.50000000e-01,
            0.00000000e+00, 7.50000000e-01, 0.00000000e+00,
            0.00000000e+00, 7.50000000e-01, 2.50000000e-01,
            1.00000000e+00, 7.50000000e-01, 5.00000000e-01,
            0.00000000e+00, 7.50000000e-01, 7.50000000e-01,
            2.50000000e-01, 0.00000000e+00, 0.00000000e+00,
            2.50000000e-01, 0.00000000e+00, 2.50000000e-01,
            2.50000000e-01, 1.09651657e-16, 5.00000000e-01,
            2.50000000e-01, 0.00000000e+00, 7.50000000e-01,
            2.50000000e-01, 2.50000000e-01, 0.00000000e+00,
            2.50000000e-01, 2.50000000e-01, 2.50000000e-01,
            2.50000000e-01, 2.50000000e-01, 5.00000000e-01,
            2.50000000e-01, 2.50000000e-01, 7.50000000e-01,
            2.50000000e-01, 5.00000000e-01, 5.48258284e-17,
            2.50000000e-01, 5.00000000e-01, 2.50000000e-01,
            2.50000000e-01, 5.00000000e-01, 5.00000000e-01,
            2.50000000e-01, 5.00000000e-01, 7.50000000e-01,
            2.50000000e-01, 7.50000000e-01, 0.00000000e+00,
            2.50000000e-01, 7.50000000e-01, 2.50000000e-01,
            2.50000000e-01, 7.50000000e-01, 5.00000000e-01,
            2.50000000e-01, 7.50000000e-01, 7.50000000e-01,
            5.00000000e-01, 0.00000000e+00, 0.00000000e+00,
            5.00000000e-01, 5.48258284e-17, 2.50000000e-01,
            5.00000000e-01, 0.00000000e+00, 5.00000000e-01,
            5.00000000e-01, 0.00000000e+00, 7.50000000e-01,
            5.00000000e-01, 2.50000000e-01, 2.74129142e-17,
            5.00000000e-01, 2.50000000e-01, 2.50000000e-01,
            5.00000000e-01, 2.50000000e-01, 5.00000000e-01,
            5.00000000e-01, 2.50000000e-01, 7.50000000e-01,
            5.00000000e-01, 5.00000000e-01, 0.00000000e+00,
            5.00000000e-01, 5.00000000e-01, 2.50000000e-01,
            5.00000000e-01, 5.00000000e-01, 5.00000000e-01,
            5.00000000e-01, 5.00000000e-01, 7.50000000e-01,
            5.00000000e-01, 7.50000000e-01, 0.00000000e+00,
            5.00000000e-01, 7.50000000e-01, 2.50000000e-01,
            5.00000000e-01, 7.50000000e-01, 5.00000000e-01,
            5.00000000e-01, 7.50000000e-01, 7.50000000e-01,
            7.50000000e-01, 0.00000000e+00, 0.00000000e+00,
            7.50000000e-01, 0.00000000e+00, 2.50000000e-01,
            7.50000000e-01, 1.00000000e+00, 5.00000000e-01,
            7.50000000e-01, 0.00000000e+00, 7.50000000e-01,
            7.50000000e-01, 2.50000000e-01, 0.00000000e+00,
            7.50000000e-01, 2.50000000e-01, 2.50000000e-01,
            7.50000000e-01, 2.50000000e-01, 5.00000000e-01,
            7.50000000e-01, 2.50000000e-01, 7.50000000e-01,
            7.50000000e-01, 5.00000000e-01, 0.00000000e+00,
            7.50000000e-01, 5.00000000e-01, 2.50000000e-01,
            7.50000000e-01, 5.00000000e-01, 5.00000000e-01,
            7.50000000e-01, 5.00000000e-01, 7.50000000e-01,
            7.50000000e-01, 7.50000000e-01, 0.00000000e+00,
            7.50000000e-01, 7.50000000e-01, 2.50000000e-01,
            7.50000000e-01, 7.50000000e-01, 5.00000000e-01,
            7.50000000e-01, 7.50000000e-01, 7.50000000e-01
    });

    std::vector<std::string> species { "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Al", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni", "Ni" };
    /*std::vector<std::string> species { "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Cs", "Cl", "Ne", "Ne", "Ne", "Ne", "Ne", "Ne", "Ne", "Ne" };*/
    std::array<bool, 3> pbc {true, true, true};

    Structure structure(lattice, frac_coords, species, pbc);

    pair_shell_weights_t shell_weights {
            {4, 0.25},
           {5, 0.2},
            {1, 1.0},
           {3, 0.33},
           {2, 0.5},
            {6, 1.0/6.0},
            {7, 1.0/7.0}
    };

    array_2d_t pair_weights = boost::make_multi_array<double, 2, 2>({
        1.0, 2.0, 5.0, 2.0
    });


    /* array_2d_t pair_weights = boost::make_multi_array<double, 4, 4>({
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0
    });*/
    auto conf (structure.configuration());

    array_3d_t target_objective(boost::extents[shell_weights.size()][nspecies][nspecies]);
    //std::fill(target_objective.begin(), target_objective.end(), 0.0);

    auto niteration {500000};
    //omp_set_num_threads(1);
    IterationSettings settings(structure, target_objective, pair_weights, shell_weights, niteration, 10, {2, 2, 2});
    settings.shell_matrix();
    auto initial_rank = rank_permutation(settings.packed_configuraton(), settings.num_species());

    std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
    int mpi_threading_support_level;
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_SERIALIZED, &mpi_threading_support_level);
    do_pair_iterations(settings);
    MPI_Finalize();
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::cout << "Time difference = " << static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count())/niteration << " [µs]" << std::endl;
    auto f = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    std::cout << f << std::endl;
}


