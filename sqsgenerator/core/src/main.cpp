//
// Created by dominik on 21.05.21.
#include <iostream>
#include <thread>
#include <vector>
#include "types.hpp"
#include "containers.hpp"
#include "sqs.hpp"
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>

using namespace boost::numeric::ublas;

void print_conf(const sqsgenerator::Configuration &conf) {
    std::cout << "[";
    for (auto s: conf) {
        std::cout << s << " ";
    }
    std::cout << conf[conf.size() - 1] << "]" << std::endl;
}

void print_array(const boost::const_multi_array_ref<double,2> &mat, size_t rows, size_t cols) {
    std::cout << "[";
    for (size_t i = 0; i < rows; i++) {
        std::cout << "[";
        for (size_t j = 0; j < cols - 1; j++) {
            std::cout << mat[i][j] << ", ";
        }
        std::cout << mat[i][cols- 1] << "]";
        if (i < rows - 1) std::cout << std::endl;
    }
    std::cout << "]" << std::endl;
}

template<typename T>
void print_matrix1(const matrix<T> &mat) {
    std::cout << "[";
    for (size_t i = 0; i < mat.size1(); i++) {
        std::cout << "[";
        for (size_t j = 0; j < mat.size2() - 1; j++) {
            std::cout << mat(i, j) << ", ";
        }
        std::cout << mat(i, mat.size2() - 1) << "]";
        if (i < mat.size1() - 1) std::cout << std::endl;
    }
    std::cout << "]" << std::endl;
}

template<typename T>
T n2(T x, T y, T z) {
    return std::sqrt(x*x+y*y+z*z);
}

int main(int argc, char *argv[]) {
    (void) argc, (void) argv;
    using namespace sqsgenerator;


    size_t nshells{7}, nspecies{2};
    std::vector<double> data{0, 1, 1, 0, 0, 2, 2, 0, 0, 3, 3, 0, 0, 4, 4, 0, 0, 5, 5, 0, 0, 6, 6, 0, 0, 7, 7, 0};

    Configuration conf{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};
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
    count_pairs<multi_array_ref<double, 3>>(conf, sro_params_ref);
}
//

