//
// Created by dominik on 27.05.21.
//

#include <unordered_set>

#include "sqsgen/core/permutation.h"
#include <gtest/gtest.h>


namespace sqsgen::testing {
    using namespace sqsgen::core;


    class RankTestFixture : public ::testing::Test {
    protected:
        configuration_t empty{};
        configuration_t conf_first_2{0, 1};
        configuration_t conf_last_2{1, 0};

        configuration_t conf_first_16{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};
        configuration_t conf_last_16{1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};

        configuration_t conf_last_24{2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
        configuration_t conf_first_24{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2};

        configuration_t conf_diff_20{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
        configuration_t conf_same_20{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        std::vector<configuration_t> first_permutations{
            empty,
            conf_first_2,
            conf_first_16,
            conf_first_24,
            conf_same_20,
            conf_diff_20
        };

        std::vector<configuration_t> last_permutations{
            empty,
            conf_last_2,
            conf_last_16,
            conf_last_24,
            conf_same_20
        };
    };

    TEST_F(RankTestFixture, test_num_permutations_empty) {
        ASSERT_EQ(num_permutations(empty), 1);
    }

    TEST_F(RankTestFixture, test_num_permutations) {
        ASSERT_EQ(num_permutations(conf_first_2), 2);
        ASSERT_EQ(num_permutations(conf_last_2), 2);
        ASSERT_EQ(num_permutations(conf_first_16), 12870);
        ASSERT_EQ(num_permutations(conf_last_16), 12870);
        ASSERT_EQ(num_permutations(conf_first_24), 9465511770);
        ASSERT_EQ(num_permutations(conf_last_24), 9465511770);
        ASSERT_EQ(num_permutations(conf_diff_20), rank_t {2432902008176640000});
        ASSERT_EQ(num_permutations(conf_same_20), 1);
    }


    TEST_F(RankTestFixture, test_rank_permutation_first) {
        for (const configuration_t &conf: first_permutations) {
            ASSERT_EQ(rank_permutation(conf), rank_t{1});
        }
    }

    TEST_F(RankTestFixture, test_rank_permutation_last) {
        for (const configuration_t &conf: last_permutations) {
            ASSERT_EQ(rank_permutation(conf), num_permutations(conf));
        }
    }


    TEST_F(RankTestFixture, test_unrank_permutation_first) {
        for (const configuration_t &conf: first_permutations) {
            ASSERT_EQ(unrank_permutation(conf, rank_t{1}), conf);
            ASSERT_EQ(rank_permutation(unrank_permutation(conf, rank_t{1})), rank_t {1});
        }
    }

    TEST_F(RankTestFixture, test_unrank_permutation_last) {
        for (const configuration_t &conf: last_permutations) {
            ASSERT_EQ(unrank_permutation(conf, num_permutations(conf)), conf);
            ASSERT_EQ(rank_permutation(unrank_permutation(conf, num_permutations(conf))), num_permutations(conf));
        }
    }

    TEST_F(RankTestFixture, test_unrank_permutation_all) {
        auto freqs = count_species(conf_first_16);
        for (auto i = 1; i <= num_permutations(conf_first_16); i++) {
            ASSERT_EQ(rank_permutation(unrank_permutation(conf_first_16, i)), i);
            ASSERT_EQ(rank_permutation(unrank_permutation(conf_first_16, freqs, i)), i);
        }
    }


        TEST_F(RankTestFixture, test_next_permutation) {
            configuration_t output(conf_first_16);
            for (auto i = 1; i < num_permutations(conf_first_16); i++) {
                ASSERT_EQ(output, unrank_permutation(conf_first_16, rank_t{i}));
                bool result {next_permutation(output)};
                ASSERT_EQ(result, true);
                ASSERT_EQ(rank_permutation(output), rank_t{i + 1});

            }
            ASSERT_EQ(rank_permutation(output), num_permutations(output));
        }

}
