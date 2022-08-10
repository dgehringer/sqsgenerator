//
// Created by dominik on 08.07.21.
//

#include "utils.hpp"
#if defined(_WIN32) || defined(_WIN64)
#include "uint128_t.h"
#define UINT128_T uint128_t
#else
#define UINT128_T __uint128_t
#endif

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
        for (size_t i = 0; i < uspcies.size(); i++) {
            hist[i] = std::count(conf.begin(), conf.end(), uspcies[i]);
        }
        return hist;
    }

    std::tuple<configuration_t, configuration_t> pack_configuration(const configuration_t &configuration) {
        auto unique_spec { unique_species(configuration) };
        std::sort(unique_spec.begin(), unique_spec.end());
        configuration_t remapped(configuration);
        for (size_t i = 0; i < configuration.size(); i++) {
            int index { get_index(unique_spec, configuration[i]) };
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
        UINT128_T tmp;
        tmp = (UINT128_T)*seed * UINT64_C(0xa3b195354a39b70d);
        uint64_t m1 = (tmp >> 64) ^ tmp;
        tmp = (UINT128_T)m1 * UINT64_C(0x1b03738712fad5c9);
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
    void shuffle_configuration_(configuration_t &configuration, uint64_t *seed) {
        for (uint32_t i=configuration.size(); i > 1; i--) {
            uint32_t p = random_bounded(i, seed); // number in [0,i)
            std::swap(configuration[i-1], configuration[p]); // swap the values at i-1 and p
        }
    }

    // Simple Knuth-Fisher-Yates shuffle with random generator
    void shuffle_configuration(configuration_t &configuration, uint64_t *seed, const shuffling_bounds_t &bounds) {
        for (auto &bound : bounds) {
            auto [lower_bound, upper_bound] = bound;
            auto window_size = upper_bound - lower_bound;
            for (uint32_t i = window_size; i > 1; i--) {
                uint32_t p = random_bounded(i, seed); // number in [0,i)
                std::swap(configuration[lower_bound+i-1], configuration[p+lower_bound]); // swap the values at i-1 and p
            }
        }
    }

    std::set<species_t> compute_occupied_sublattices(const composition_t &composition) {
        std::set<species_t> used_sublattices;
        for (auto const&[_, sublattice_spec]: composition)
            for (auto const &[destination_sublattice, __]: sublattice_spec)
                used_sublattices.insert(destination_sublattice);
        return used_sublattices;
    }

    shuffling_bounds_t compute_shuffling_bounds(const configuration_t &initial, const composition_t &composition) {
        auto unique_spec_initial(unique_species(initial));
        auto hist_initial(configuration_histogram(initial));
        auto used_sublattices(compute_occupied_sublattices(composition));

        shuffling_bounds_t bounds;

        size_t lower_bound = 0;
        for (auto i = 0; i < unique_spec_initial.size(); i++) {
            size_t num_atoms_on_sublattice {hist_initial[i]};
            size_t upper_bound {lower_bound + num_atoms_on_sublattice};
            // if the current sublattice is not occupied we do not shuffle this region of the configuration array
            // therefore, we check if the current sublattice is used at all
            if (used_sublattices.count(unique_spec_initial[i]))
                bounds.emplace_back(std::make_tuple(lower_bound, upper_bound));
            lower_bound += num_atoms_on_sublattice;
        }

        return bounds;
    }

    shuffling_bounds_t default_shuffling_bounds(const configuration_t &initial) {
        return { {0, initial.size()} };
    }

    bool need_sublattice_bounds(const composition_t &composition) {
        auto used_sublattices(compute_occupied_sublattices(composition));
        if (used_sublattices.size() == 1) return *used_sublattices.begin() != ALL_SITES;
        return true;
    }

    std::tuple<arrangement_t, arrangement_t, configuration_t, shuffling_bounds_t> build_configuration(const configuration_t &initial, const composition_t &composition) {
        if (composition.empty()) throw std::invalid_argument("composition map cannot be empty");

        auto rearrange_forward = sqsgenerator::utils::argsort(initial);
        auto rearrange_backward = sqsgenerator::utils::argsort(rearrange_forward);
        auto unique_spec_initial(unique_species(initial));
        auto hist_initial(configuration_histogram(initial));
        auto is_constrained = need_sublattice_bounds(composition);
        auto bounds (is_constrained ? compute_shuffling_bounds(initial, composition) : default_shuffling_bounds(initial));

        std::set<size_t> used_sublattices;
        std::vector<configuration_t> sublattice_configurations(is_constrained ? unique_spec_initial.size() : 1);

        for (auto const&[distribute_species, sublattice_spec]: composition) {
            if (sublattice_spec.empty())
                throw std::invalid_argument("composition spec for element \"Z=" +
                std::to_string(distribute_species)+"\" cannot be an empty map");
            for (auto const &[destination_sublattice, distribute_num_atoms]: sublattice_spec) {
                if (is_constrained && destination_sublattice == ALL_SITES)
                    throw std::invalid_argument(
                        "constrained composition specs, thus I cannot distribute \"Z=" +
                        std::to_string(distribute_species) +
                        "\" on all available sites. "
                        "You must explicitly specify on which sublattice those atoms must be placed"
                    );
                int index {destination_sublattice == ALL_SITES ? 0 : get_index(unique_spec_initial, destination_sublattice)};
                if (index < 0)
                    throw std::runtime_error("The sublattice \"Z=" + std::to_string(destination_sublattice) +
                    "\" is not specified in the initial structure");
                for (auto i = 0; i < distribute_num_atoms; i++) sublattice_configurations[index].push_back(distribute_species);
            }
        }
        // There might be the case that there is a sublattice on which no atoms were distributes
        // In this case we fill it up with the species itself
        configuration_t final_configuration;
        if (is_constrained) {
            for (auto i = 0; i < unique_spec_initial.size(); i++) {
                auto conf = sublattice_configurations[i];
                if (conf.empty()) conf.resize(hist_initial[i], unique_spec_initial[i]);
                if (conf.size() != hist_initial[i]) {
                    throw std::runtime_error(
                        "Wrong number of atoms distributed on sublattice "
                         "\"Z=" + std::to_string(unique_spec_initial[i]) + "\". Should be " +
                         std::to_string(hist_initial[i]) + " but is " +
                         std::to_string(conf.size())
                     );
                }
                final_configuration.insert(final_configuration.end(), conf.begin(), conf.end());
            }
        }
        else {
            final_configuration.assign(sublattice_configurations[0].begin(), sublattice_configurations[0].end());
            if (final_configuration.size() != initial.size())
                throw std::runtime_error(
                    "Wrong number of atoms on lattice positions. Should be " +
                    std::to_string(initial.size()) + " but is " + std::to_string(final_configuration.size())
                );
        }

        // the configuration is intrinsically sorted, therefore we have to sort it backwards
        auto rearranged_configuration(sqsgenerator::utils::rearrange(final_configuration, rearrange_backward));
        return std::make_tuple(rearrange_forward, rearrange_backward, rearranged_configuration, bounds);
    }
}