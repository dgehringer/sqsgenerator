//
// Created by dominik on 27.05.21.
//

#include <gtest/gtest.h>
#include <rank.h>
#include <types.h>

namespace sqsgenerator {

    class RankTestFixture : public ::testing::Test {
    protected:
        Configuration empty {};
        Configuration conf_first_2 {0,1};
        Configuration conf_last_2 {1,0};

        Configuration conf_first_16 {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};
        Configuration conf_last_16 {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0};

        Configuration conf_last_24 {2,2,2,2,2,2,2,2, 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0};
        Configuration conf_first_24 {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2};

        Configuration conf_diff_20 {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
        Configuration conf_same_20 {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

        std::vector<Configuration> first_permutations {
                empty,
                conf_first_2,
                conf_first_16,
                conf_first_24,
                conf_same_20,
                conf_diff_20
        };

        std::vector<Configuration> last_permutations {
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
        ASSERT_EQ(total_permutations(conf_diff_20), mpz_class{2432902008176640000});
        ASSERT_EQ(total_permutations(conf_same_20), 1);
    }

    TEST_F(RankTestFixture, TestUniqueSpecies) {
        ASSERT_EQ(unique_species(empty).size(), 0);
        ASSERT_EQ(unique_species(conf_first_2).size(), 2);
        ASSERT_EQ(unique_species(conf_first_16).size(), 2);
        ASSERT_EQ(unique_species(conf_first_24).size(), 3);
        ASSERT_EQ(unique_species(conf_diff_20).size(), conf_diff_20.size());
    }

    TEST_F(RankTestFixture, TestRankPermutationFirst) {

        for (const Configuration  &conf : first_permutations) {
            size_t num_species = unique_species(conf).size();
            ASSERT_EQ(rank_permutation_std(conf, num_species), 1);
            ASSERT_EQ(rank_permutation_gmp(conf, num_species), mpz_class {1});
        }
    }

    TEST_F(RankTestFixture, TestRankPermutationLast) {
        for (const Configuration  &conf : last_permutations) {
            size_t num_species = unique_species(conf).size();
            mpz_class total_perms = total_permutations(conf);
            ASSERT_EQ(rank_permutation_std(conf, num_species), total_perms.get_ui());
            ASSERT_EQ(rank_permutation_gmp(conf, num_species), total_perms);
        }
    }

    TEST_F(RankTestFixture, TestConfigurationHistogram){
        ASSERT_EQ(configuration_histogram(empty), std::vector<size_t>());
        ASSERT_EQ(configuration_histogram(conf_first_2), std::vector<size_t>({1,1}));
        ASSERT_EQ(configuration_histogram(conf_first_16), std::vector<size_t>({8,8}));
        ASSERT_EQ(configuration_histogram(conf_first_24), std::vector<size_t>({8,8,8}));
        ASSERT_EQ(configuration_histogram(conf_same_20), std::vector<size_t>({conf_same_20.size()}));
        ASSERT_EQ(configuration_histogram(conf_diff_20), std::vector<size_t>(conf_diff_20.size(), 1));
    }

    TEST_F(RankTestFixture, TestUnrankPermutationFirst) {
        for (const Configuration &conf : first_permutations) {
            Configuration conf_copy {conf};
            unrank_permutation_gmp(conf_copy, configuration_histogram(conf_copy), total_permutations(conf_copy), mpz_class{1});
            ASSERT_EQ(conf_copy, conf);
            uint64_t total_perms_std = total_permutations(conf_copy).get_ui();
            unrank_permutation_std(conf_copy, configuration_histogram(conf_copy), total_perms_std, 1);
            ASSERT_EQ(conf_copy, conf);
            ASSERT_EQ(rank_permutation_gmp(conf_copy, unique_species(conf_copy).size()), mpz_class {1});
            ASSERT_EQ(rank_permutation_std(conf_copy, unique_species(conf_copy).size()), 1);
        }
    }

    TEST_F(RankTestFixture, TestUnrankPermutationLast) {
        for (const Configuration &conf : last_permutations) {
            Configuration conf_copy {conf};
            unrank_permutation_gmp(conf_copy, configuration_histogram(conf_copy), total_permutations(conf_copy), total_permutations(conf_copy));
            ASSERT_EQ(conf_copy, conf);
            uint64_t total_perms_std = total_permutations(conf_copy).get_ui();
            unrank_permutation_std(conf_copy, configuration_histogram(conf_copy), total_perms_std, total_perms_std);
            ASSERT_EQ(conf_copy, conf);
            ASSERT_EQ(rank_permutation_gmp(conf_copy, unique_species(conf_copy).size()), total_permutations(conf_copy));
            ASSERT_EQ(rank_permutation_std(conf_copy, unique_species(conf_copy).size()), total_perms_std);
        }
    }

    TEST_F(RankTestFixture, TestUnrankPermutationAllStd) {
        uint64_t total_perms {total_permutations(conf_first_16).get_ui()};
        Configuration local_conf {conf_first_16};
        auto hist {configuration_histogram(conf_first_16)};
        size_t nspecies {unique_species(conf_first_16).size()};

        for (uint64_t i = 1; i <= total_perms; i++) {
            unrank_permutation_std(local_conf, hist, total_perms, i);
            ASSERT_EQ(rank_permutation_std(local_conf, nspecies), i);
        }
    }

    TEST_F(RankTestFixture, TestUnrankPermutationAllGmp) {
        mpz_class total_perms {total_permutations(conf_first_16)};
        Configuration local_conf {conf_first_16};
        auto hist {configuration_histogram(conf_first_16)};
        size_t nspecies {unique_species(conf_first_16).size()};

        for (uint64_t i = 1; i <= total_perms; i++) {
            unrank_permutation_gmp(local_conf, hist, total_perms, mpz_class {i});
            ASSERT_EQ(rank_permutation_gmp(local_conf, nspecies), mpz_class {i});
        }
    }

}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}