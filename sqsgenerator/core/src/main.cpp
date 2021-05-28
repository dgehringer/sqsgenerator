//
// Created by dominik on 21.05.21.

#include <iostream>
#include <rigtorp/MPMCQueue.h>
#include <thread>
#include <types.h>
#include <vector>
#include <algorithm>
#include <rank.h>
#include <utils.h>
#include <xoroshiro256.h>

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

    for (int i = 0; i < 40; i++) {
        std::cout << bounded_rand(rng, 40) << ", "
    }
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

