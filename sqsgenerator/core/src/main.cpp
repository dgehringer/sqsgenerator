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

   Configuration conf {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};
   PairSROParameters params(boost::extents[nshells][nspecies][nspecies]);
   SQSResult<PairSROParameters> result(0.0, 1, conf, params);

   SQSResultCollection<SQSResult<PairSROParameters>> results(5);
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
   });


   generator.join();
    std::cout << results.size() << std::endl;
}
//

