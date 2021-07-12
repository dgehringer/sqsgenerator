//
// Created by dominik on 27.05.21.
//

#include "types.hpp"
#include "rank.hpp"
#include "utils.hpp"
#include <gtest/gtest.h>

using namespace sqsgenerator::utils;

namespace sqsgenerator::test {

    class RankTestFixture : public ::testing::Test {
    protected:
        configuration_t empty {};
        configuration_t conf_first_2 {0,1};
        configuration_t conf_last_2 {1,0};

        configuration_t conf_first_16 {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};
        configuration_t conf_last_16 {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0};

        configuration_t conf_last_24 {2,2,2,2,2,2,2,2, 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0};
        configuration_t conf_first_24 {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2};

        configuration_t conf_diff_20 {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
        configuration_t conf_same_20 {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

        std::vector<configuration_t> first_permutations {
                empty,
                conf_first_2,
                conf_first_16,
                conf_first_24,
                conf_same_20,
                conf_diff_20
        };

        std::vector<configuration_t> last_permutations {
                empty,
                conf_last_2,
                conf_last_16,
                conf_last_24,
                conf_same_20
        };
    };

    TEST_F(RankTestFixture, TestTotalPermutationsEmpty) {
        ASSERT_EQ(total_permutations(empty), 1);
    }

    TEST_F(RankTestFixture, TestTotalPermutations) {
        ASSERT_EQ(total_permutations(conf_first_2), 2);
        ASSERT_EQ(total_permutations(conf_last_2), 2);
        ASSERT_EQ(total_permutations(conf_first_16), 12870);
        ASSERT_EQ(total_permutations(conf_last_16), 12870);
        ASSERT_EQ(total_permutations(conf_first_24), 9465511770);
        ASSERT_EQ(total_permutations(conf_last_24), 9465511770);
        ASSERT_EQ(total_permutations(conf_diff_20), cpp_int {2432902008176640000});
        ASSERT_EQ(total_permutations(conf_same_20), 1);
    }

    /*
    TEST_F(RankTestFixture, TestUniqueSpecies) {
        ASSERT_EQ(unique_species(empty).size(), 0);
        ASSERT_EQ(unique_species(conf_first_2).size(), 2);
        ASSERT_EQ(unique_species(conf_first_16).size(), 2);
        ASSERT_EQ(unique_species(conf_first_24).size(), 3);
        ASSERT_EQ(unique_species(conf_diff_20).size(), conf_diff_20.size());
    }*/

    TEST_F(RankTestFixture, TestRankPermutationFirst) {

        for (const configuration_t  &conf : first_permutations) {
            size_t num_species = unique_species(conf).size();
            ASSERT_EQ(rank_permutation(conf, num_species), 1);
        }
    }

    TEST_F(RankTestFixture, TestRankPermutationLast) {
        for (const configuration_t  &conf : last_permutations) {
            size_t num_species = unique_species(conf).size();
            cpp_int total_perms = total_permutations(conf);
            ASSERT_EQ(rank_permutation(conf, num_species), total_perms);
        }
    }

    /*
    TEST_F(RankTestFixture, TestConfigurationHistogram){
        ASSERT_EQ(configuration_histogram(empty), std::vector<size_t>());
        ASSERT_EQ(configuration_histogram(conf_first_2), std::vector<size_t>({1,1}));
        ASSERT_EQ(configuration_histogram(conf_first_16), std::vector<size_t>({8,8}));
        ASSERT_EQ(configuration_histogram(conf_first_24), std::vector<size_t>({8,8,8}));
        ASSERT_EQ(configuration_histogram(conf_same_20), std::vector<size_t>({conf_same_20.size()}));
        ASSERT_EQ(configuration_histogram(conf_diff_20), std::vector<size_t>(conf_diff_20.size(), 1));
    }*/

    TEST_F(RankTestFixture, TestUnrankPermutationFirst) {
        for (const configuration_t &conf : first_permutations) {
            configuration_t conf_copy {conf};
            unrank_permutation(conf_copy, configuration_histogram(conf_copy), total_permutations(conf_copy), cpp_int {1});
            ASSERT_EQ(conf_copy, conf);
            ASSERT_EQ(rank_permutation(conf_copy, unique_species(conf_copy).size()), cpp_int {1});
        }
    }

    TEST_F(RankTestFixture, TestUnrankPermutationLast) {
        for (const configuration_t &conf : last_permutations) {
            configuration_t conf_copy {conf};
            unrank_permutation(conf_copy, configuration_histogram(conf_copy), total_permutations(conf_copy), total_permutations(conf_copy));
            ASSERT_EQ(conf_copy, conf);
            ASSERT_EQ(rank_permutation(conf_copy, unique_species(conf_copy).size()), total_permutations(conf_copy));
        }
    }

    TEST_F(RankTestFixture, TestUnrankPermutationAllStd) {
        configuration_t local_conf {conf_first_16};
        auto hist {configuration_histogram(conf_first_16)};
        size_t nspecies {unique_species(conf_first_16).size()};
        auto total_perms = total_permutations(conf_first_16);
        for (uint64_t i = 1; i <= total_perms; i++) {
            unrank_permutation(local_conf, hist, total_perms, i);
            ASSERT_EQ(rank_permutation(local_conf, nspecies), i);
        }
    }

    TEST_F(RankTestFixture, TestNextPermutation) {
        configuration_t local_conf(conf_first_16);
        configuration_t local_conf_unrank(conf_first_16);
        auto hist {configuration_histogram(conf_first_16)};
        size_t nspecies {unique_species(conf_first_16).size()};
        auto total_perms = total_permutations(conf_first_16);
        for (rank_t i = 1; i < total_perms; i++) {
            ASSERT_EQ(rank_permutation(local_conf, nspecies), i);
            // Also increase permuation by one
            unrank_permutation(local_conf_unrank, hist, total_perms, i);
            ASSERT_EQ(local_conf, local_conf_unrank);
            bool result {next_permutation(local_conf)};
            ASSERT_EQ(result, true);

        }
        ASSERT_EQ(rank_permutation(local_conf, nspecies), total_perms);
        ASSERT_EQ(next_permutation(local_conf), false);
    }


}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}