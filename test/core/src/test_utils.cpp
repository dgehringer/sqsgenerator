//
// Created by dominik on 09.08.22.
//

#include <map>
#include "types.hpp"
#include "utils.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>

#if defined(_WIN32) || defined(_WIN64)
#include "uint128_t.h"
#define UINT128_T uint128_t
#else
#define UINT128_T __uint128_t
#endif

using namespace sqsgenerator::utils;

namespace sqsgenerator::test {

    typedef std::map<species_t, int> species_map_t;

    /* this part was taken out as inline random_bounded_non_inline */
    static inline uint64_t wyhash64_stateless(uint64_t *seed) {
        *seed += UINT64_C(0x60bee2bee120fc15);
        UINT128_T tmp;
        tmp = (UINT128_T)*seed * UINT64_C(0xa3b195354a39b70d);
        uint64_t m1 = (tmp >> 64) ^ tmp;
        tmp = (UINT128_T)m1 * UINT64_C(0x1b03738712fad5c9);
        uint64_t m2 = (tmp >> 64) ^ tmp;
        return m2;
    }

    uint32_t random_bounded_non_inline(uint32_t range, uint64_t *seed) {
        uint32_t x = wyhash64_stateless(seed);
        uint64_t m = uint64_t(x) * uint64_t(range);
        return m >> 32;
    }

    class UtilsTestFixture : public ::testing::Test {

    protected:
        uint64_t m_seed;

    public:
        void SetUp() {
            m_seed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            std::cout << "sqsgenerator::test::seed=" << m_seed << std::endl;
        };
        void TearDown() {}

        uint32_t random_bounded(uint32_t range) {
            return random_bounded_non_inline(range, &m_seed);
        }

        uint32_t random_range(uint32_t minimum, uint32_t maximum) {
            return random_bounded((maximum - minimum)) + minimum;
        }

        species_t random_species() {
            return static_cast<species_t>(random_range(1, 90));
        }

        std::pair<species_map_t, configuration_t> build_random_configuration(int num_atoms, int num_species) {
            auto sum_atoms {0};
            species_map_t histogram;
            configuration_t configuration(num_atoms);

            for (auto i = 0; i < num_species; i++) {
                species_t ordinal { random_species() };
                while (histogram.count(ordinal)) { ordinal = random_species(); }
                auto atoms_of_species {random_range(1, num_atoms / num_species) };

                std::fill(configuration.begin() + sum_atoms, configuration.begin() + sum_atoms + atoms_of_species, ordinal);
                histogram.emplace(std::make_pair(ordinal, atoms_of_species));
                sum_atoms += atoms_of_species;

            }

            auto remaining_atoms {num_atoms - sum_atoms}, remaining_species {random_species()};
            while (histogram.count(remaining_species)) { remaining_species = random_species(); }
            histogram.emplace(std::make_pair(remaining_species, remaining_atoms));
            std::fill(configuration.begin() + sum_atoms, configuration.begin() + sum_atoms + remaining_atoms, remaining_species);

            auto check_atoms {0};
            for (const auto &pair: histogram) check_atoms += pair.second;

            if (check_atoms != num_atoms) {
                std::cout << remaining_atoms << "/" << num_species << "/" << check_atoms << "/" << num_atoms << " : " <<format_map(histogram) << std::endl;
            }

            return std::make_pair(histogram, configuration);
        }

        std::pair<species_map_t, configuration_t> default_random_configuration(int min_num_atoms = 64, int max_num_atoms=128, int min_num_species=2, int max_num_species=7) {
            auto num_atoms { random_range(min_num_atoms, max_num_atoms) };
            auto num_species { random_range(min_num_species, max_num_species) };
            return build_random_configuration(num_atoms, num_species);
        }
    };

    TEST_F(UtilsTestFixture, TestUniqueSpecies) {
        auto num_test_cases {512};
        for (auto _ = 0; _ < num_test_cases; _++) {
            auto [species_map, configuration] = default_random_configuration();
            auto sorted_species = apply<species_map_t, species_t, species_map_t::value_type>(species_map, first<species_map_t::value_type>);
            std::sort(sorted_species.begin(), sorted_species.end());

            auto unique_spec = unique_species(configuration);
            ASSERT_EQ(sorted_species.size(), unique_spec.size());
            assert_vector_equals<decltype(sorted_species), decltype(unique_spec), species_t>(sorted_species, unique_spec);
        }
    }

    TEST_F(UtilsTestFixture, TestConfigurationHistogram) {
        auto num_test_cases {512};
        for (auto _ = 0; _ < num_test_cases; _++) {
            species_map_t species_map;
            configuration_t configuration;
            std::tie(species_map, configuration) = default_random_configuration();
            //auto [species_map, configuration] = default_random_configuration();
            auto unique_spec = unique_species(configuration);

            auto histogram = apply<size_t, species_t>(unique_spec, [&species_map](const species_t &spec) { return species_map[spec]; });
            auto histogram_test = configuration_histogram(configuration);

            ASSERT_EQ(std::accumulate(histogram.begin(), histogram.end(), 0), configuration.size());
            ASSERT_EQ(std::accumulate(histogram_test.begin(), histogram_test.end(), 0), configuration.size());

            ASSERT_EQ(histogram.size(), histogram_test.size());
            assert_vector_equals<decltype(histogram), decltype(histogram_test), size_t>(histogram, histogram_test);
        }
    }
}



// auto current_time {std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()};