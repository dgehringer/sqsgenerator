//
// Created by dominik on 30.06.21.
//


# include <vector>
#include <cassert>
#include <chrono>
#include <iostream>
#include <boost/multi_array.hpp>

using namespace boost;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

void init_shell_matrix(multi_array_ref<int, 2> &shells, size_t num_atoms, size_t num_pairs) {
    size_t distributed_pairs {0};
    for (size_t i = 0; i < num_atoms; i++) {
        for (size_t j = i ; j < num_atoms; j++) {
            int shell_value;
            if (i == j) shell_value = 0;
            else if (distributed_pairs < num_pairs) {
                shell_value = i*num_atoms + j;
                distributed_pairs++;
            } else {
                shell_value = -1;
            }
            shells[i][j] = shell_value;
            shells[j][i] = shell_value;
        }
    }
    std::cout << "distributed_pairs = " << distributed_pairs << std::endl;
}

typedef std::tuple<size_t, size_t, size_t> AtomPair;
typedef std::vector<AtomPair> AtomPairList;
typedef std::vector<uint8_t> Configuration;

AtomPairList calculate_pair_list(multi_array_ref<int, 2> &shells, size_t num_atoms) {
    AtomPairList result;
    for (size_t i = 0; i < num_atoms; i++) {
        for (size_t j = i; j < num_atoms; j++) {
            if (shells[i][j] > 0)  result.push_back(std::make_tuple(i, j, static_cast<size_t>(shells[i][j])));
        }
    }

    return result;
}

size_t loop_naive(const configuration_t &configuration, multi_array_ref<int, 2> &shells, size_t num_loops = 10000) {
    uint8_t first_species, second_species;
    int pair_shell;
    size_t pair_counts {0};
    for (size_t loop = 0; loop < num_loops; loop++) {
        //std::cout << "loop_benchmark::loop_naive() - " << loop << std::endl;
        for (size_t i = 0; i < configuration.size() ; i++) {
            first_species = configuration[i];
            for (size_t j = i + 1; j < configuration.size() ; j++) {
                pair_shell = shells[i][j];
                if (pair_shell < 0) continue;
                second_species = configuration[j];
                pair_counts+= first_species * second_species;
            }
        }
    }
    //std::cout << "loop_benchmark::loop_naive() - " << pair_counts << std::endl;
    return pair_counts;
}

size_t loop_vector(const configuration_t &configuration, const AtomPairList &pairs, size_t num_loops = 10000) {
    uint8_t first_species, second_species;
    size_t pair_counts {0};
    size_t i, j, shell;
    for (size_t loop = 0; loop < num_loops; loop++) {
        for (const AtomPair &pair: pairs) {
            std::tie(i, j, shell) = pair;
            first_species = configuration[i];
            second_species = configuration[j];
            pair_counts+= first_species * second_species;
        }
    }
    //std::cout << "loop_benchmark::loop_vector() - " << pair_counts << std::endl;
    return pair_counts;
}

void compare_loops(size_t num_atoms, size_t frac = 2, size_t num_loops = 10000, std::string prefix = "\t") {
    size_t num_pairs {(num_atoms * (num_atoms -1))/frac};
    // std::cout << " NUM PAIRS = " << num_pairs << "(" << frac << ")" << std::endl;
    multi_array<int, 2> shells(boost::extents[num_atoms][num_atoms]);
    multi_array_ref<int, 2> shell_ref(shells.data(), boost::extents[num_atoms][num_atoms]);

    init_shell_matrix(shell_ref, num_atoms, num_pairs);
    AtomPairList pair_list(calculate_pair_list(shell_ref, num_atoms));
    assert(pair_list.size() == num_pairs);

    configuration_t conf(num_atoms);
    std::iota(conf.begin(), conf.end(), 1);

    auto t0 = high_resolution_clock::now();
    auto result_naive = loop_naive(conf, shell_ref, num_loops);
    auto t1 = high_resolution_clock::now();
    duration<double, std::milli>  time_naive = t1 - t0;
    auto loop_time_naive = time_naive.count() *1000 /num_loops;
    std::cout << prefix << "loop_benchmark::loop_naive() - " << loop_time_naive << " µs/cycle" << std::endl;

    t0 = high_resolution_clock::now();
    auto result_vector = loop_vector(conf, pair_list, num_loops);
    t1 = high_resolution_clock::now();
    duration<double, std::milli>  time_vector = t1 - t0;
    auto loop_time_vector = time_vector.count() *1000 /num_loops;
    std::cout << prefix << "loop_benchmark::loop_vector() - " << loop_time_vector << " µs/cycle" << std::endl;
    std::cout << prefix << "loop_naive()/loop_vector() - " << loop_time_naive / loop_time_vector<< std::endl;
    assert(result_naive == result_vector);
}

int main(int argc, char *argv[]) {
    std::vector<size_t> fractions {2, 3, 5, 6, 9, 10, 15, 20, 100, 200};
    for (size_t &frac : fractions) {
        compare_loops(500, frac, 20000);
    }

}
