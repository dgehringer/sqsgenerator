//
// Created by dominik on 08.07.21.
//

#include "utils.hpp"

namespace sqsgenerator::utils {

    configuration_t unique_species(configuration_t conf) {
        // it is intended that we do not pass in by-reference since we want to have a copy of the configuration
        std::sort( conf.begin(), conf.end() );
        conf.erase( std::unique( conf.begin(), conf.end() ), conf.end() );
        return conf;
    }

    std::vector<size_t> configuration_histogram(const configuration_t &conf) {
        auto uspcies = unique_species(conf);
        std::vector<size_t> hist(uspcies.size());
        for (int i = 0; i < uspcies.size(); i++) {
            hist[i] = std::count(conf.begin(), conf.end(), uspcies[i]);
        }
        return hist;
    }

    std::tuple<configuration_t, configuration_t> pack_configuration(const configuration_t &configuration) {

        auto unique_spec { unique_species(configuration) };
        std::sort(unique_spec.begin(), unique_spec.end());
        configuration_t remapped(configuration);
        for (size_t i = 0; i < configuration.size(); i++) {
            int index{get_index(unique_spec, configuration[i])};
            if (index < 0) throw std::runtime_error("A species was detected which I am not aware of");
            remapped[i] = index;
        }
        return std::make_tuple(unique_spec, remapped);
    }

    configuration_t unpack_configuration(const configuration_t &indices, const configuration_t &packed) {
        configuration_t unpacked;
        for (auto &index: packed) unpacked.push_back(indices[index]);
        return unpacked;
    }


    // Implementation of the whyash algorithm for generating relatively fast random numbers
    // Seen on David Lamires blog: https://lemire.me/blog/2019/03/19/the-fastest-conventional-random-number-generator-that-can-pass-big-crush/
    // Code taken from: https://github.com/lemire/testingRNG/blob/master/source/wyhash.h
    static inline uint64_t wyhash64_stateless(uint64_t *seed) {
        *seed += UINT64_C(0x60bee2bee120fc15);
        __uint128_t tmp;
        tmp = (__uint128_t)*seed * UINT64_C(0xa3b195354a39b70d);
        uint64_t m1 = (tmp >> 64) ^ tmp;
        tmp = (__uint128_t)m1 * UINT64_C(0x1b03738712fad5c9);
        uint64_t m2 = (tmp >> 64) ^ tmp;
        return m2;
    }

    // Implementation of a bounded random number without division and modulo-operator
    // Seen on pcg-random.org blog: https://www.pcg-random.org/posts/bounded-rands.html
     inline uint32_t random_bounded(uint32_t range, uint64_t *seed) {
         uint32_t x = wyhash64_stateless(seed);
         uint64_t m = uint64_t(x) * uint64_t(range);
         return m >> 32;
    }
    // Simple Knuth-Fisher-Yates shuffle with random generator
    void shuffle_configuration(configuration_t &configuration, uint64_t *seed) {
        for (uint32_t i=configuration.size(); i > 1; i--) {
            uint32_t p = random_bounded(i, seed); // number in [0,i)
            std::swap(configuration[i-1], configuration[p]); // swap the values at i-1 and p
        }
    }
}