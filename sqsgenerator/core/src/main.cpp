//
// Created by dominik on 21.05.21.

#include <iostream>
#include <rigtorp/MPMCQueue.h>
#include <thread>
#include <types.h>
#include <vector>
#include <algorithm>
#include <rank.h>

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

    using namespace rigtorp;
    using namespace sqsgenerator;

    Configuration conf{0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};
    Configuration conf1{1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0};
    auto species = unique_species(conf);
    auto nspecies = unique_species(conf).size();
    auto total_permutation = total_permutations(conf);
    auto max_rank = rank_permutation_gmp(conf1, nspecies);
    auto hist = configuration_histogram(conf);
    std::cout << conf.size() << std::endl;
    std::cout << species.size() << std::endl;
    std::cout << total_permutations(conf) << std::endl;
    std::cout << "RANK:" << rank_permutation_gmp(conf, nspecies) << " , " << rank_permutation_gmp(conf1, nspecies) << " , "<<  mpz_class(1) <<std::endl;
    Configuration last(conf.size());
    unrank_permutation_gmp(last, hist, total_permutation, mpz_class{1567});
    std::cout << "NEW RANK: "<< rank_permutation_gmp(last, nspecies) << std::endl;
//   print_conf(last);
    MPMCQueue<int> q(10);
    auto t1 = std::thread([&] {
        int v;
        q.pop(v);
        std::cout << "t1 " << v << "\n";
    });
    auto t2 = std::thread([&] {
        int v;
        q.pop(v);
        std::cout << "t2 " << v << "\n";
    });
    q.push(1);
    q.push(2);
    t1.join();
    t2.join();

    return 0;
}
//

