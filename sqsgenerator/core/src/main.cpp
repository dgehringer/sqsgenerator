//
// Created by dominik on 21.05.21.

#include <iostream>
#include <thread>
#include <types.h>
#include <vector>
#include <algorithm>
#include <rank.h>
#include "containers.h"

void print_conf(const Configuration &conf) {
    std::cout << "[";
    for (auto s: conf)
    {
        std::cout << s << " ";
    }
    std::cout << conf[conf.size()-1] << "]" << std::endl;
}

int main(int argc, char *argv[]) {
    (void)argc, (void)argv;
    using namespace sqsgenerator;

    size_t nshells {7}, nspecies {2};
    std::vector<double> data {0,1,1,0,0,2,2,0,0,3,3,0,0,4,4,0,0,5,5,0,0,6,6,0,0,7,7,0};

   Configuration conf {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};
   PairSROParameters params(boost::extents[nshells][nspecies][nspecies]);
   params.assign(data.begin(), data.end());
   PairSQSResult result(0.0, 1, conf, params);

   PairSQSIterationResult results(5);
   size_t nthread = 8;


   std::thread generator([&](){
       for (size_t i = 1; i < 11; i++) {
           double objective = 1.0/i;
           uint64_t rank = i;
           result.rank = rank;
           result.objective = objective;
           bool res = results.addResult(result);
           std::cout << i << ": " << results.bestObjective()<< ", " << objective << ", "  << "res" << std::endl;
       }
       results.addResult(result);
       //results.addResult(result);
   });


    generator.join();
    results.collect();
    assert(results.bestObjective() == 1.0/10.0);
    std::cout << results.bestObjective() << ", " <<  results.resultSize() << std::endl;
}
//

