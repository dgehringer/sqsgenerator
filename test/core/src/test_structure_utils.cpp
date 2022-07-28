//
// Created by dominik on 29.06.21.
//

#include "test_helpers.hpp"
#include "utils.hpp"
#include "structure_utils.hpp"
#include <cmath>
#include <stdexcept>
#include <gtest/gtest.h>
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>


using namespace boost;
using namespace sqsgenerator::utils;
using namespace boost::numeric::ublas;
namespace fs = std::filesystem;

namespace sqsgenerator::test {

    class StructureUtilsTestFixture : public ::testing::Test {
    protected:
        std::vector<TestCaseData> test_cases{};

    public:
        void SetUp() {
            // code here will execute just before the test ensues
            // std::cout << "StructureUtilsTestFixture::SetUp(): " << std::filesystem::current_path() << std::endl;
            test_cases = load_test_cases("resources");
        };

        void TearDown() {
            // code here will be called just after the test completes
            // ok to through exceptions from here if need be
        }
    };

    TEST_F(StructureUtilsTestFixture, TestPbcVectors) {
        for (TestCaseData &test_case : test_cases) {
            matrix<double> lattice (matrix_from_multi_array(test_case.lattice));
            matrix<double> fcoords (matrix_from_multi_array(test_case.fcoords));
            auto pbc_vecs = sqsgenerator::utils::pbc_shortest_vectors(lattice, fcoords, true);
            for (size_t i = 0; i < 3; i++) ASSERT_EQ(pbc_vecs.shape()[i], test_case.vecs.shape()[i]);

            assert_multi_array_near<decltype(pbc_vecs), decltype(test_case.vecs), double>(pbc_vecs, test_case.vecs, std::abs<double>, std::abs<double>);
        }
    }

    TEST_F(StructureUtilsTestFixture, TestDistanceMatrix) {
        typedef multi_array<double, 3> pbc_mat;
        for (TestCaseData &test_case : test_cases) {
            matrix<double> lattice (matrix_from_multi_array(test_case.lattice));
            matrix<double> fcoords (matrix_from_multi_array(test_case.fcoords));
            auto pbc_vecs = sqsgenerator::utils::pbc_shortest_vectors(lattice, fcoords, true);
            auto d2 = sqsgenerator::utils::distance_matrix(pbc_vecs);

            for (size_t i = 0; i < 2; i++)  ASSERT_EQ(d2.shape()[i], test_case.vecs.shape()[i]);
            assert_multi_array_near<decltype(d2), decltype(test_case.distances), double>(d2, test_case.distances);

            auto d2_external = sqsgenerator::utils::distance_matrix(test_case.vecs);
            assert_multi_array_near<decltype(d2), decltype(d2_external), double>(d2, d2_external);
        }
    }

    TEST_F(StructureUtilsTestFixture, TestShellMatrix) {
        for (TestCaseData &test_case : test_cases) {
            matrix<double> lattice (matrix_from_multi_array(test_case.lattice));
            matrix<double> fcoords (matrix_from_multi_array(test_case.fcoords));
            auto pbc_vecs = sqsgenerator::utils::pbc_shortest_vectors(lattice, fcoords, true);
            auto d2 = sqsgenerator::utils::distance_matrix(pbc_vecs);
            auto distances = sqsgenerator::utils::default_shell_distances(d2, 1.0e-3);
            auto shells = sqsgenerator::utils::shell_matrix(d2, distances, 1.0e-3);
            auto natoms {static_cast<index_t>(fcoords.size1())};

            for (size_t i = 0; i < 2; i++)  ASSERT_EQ(shells.shape()[i], test_case.shells.shape()[i]);
            assert_multi_array_near<decltype(shells), decltype(test_case.shells), shell_t>(shells, test_case.shells);

            auto shells_external = sqsgenerator::utils::shell_matrix(test_case.distances, distances);
            assert_multi_array_near<decltype(shells), decltype(shells_external), shell_t>(shells, shells_external);

            // Make sure the main diagonal is zero
            for (index_t i = 0; i < natoms; i++) {
                ASSERT_EQ(shells[i][i], 0);
            }
            // Ensure the matrix is symmetric
            for (index_t i = 0; i < natoms; i++) {
                for (index_t j = i+1; j < natoms; j++)
                    ASSERT_EQ(shells[i][j], shells[j][i]);
            }
        }
    }

    TEST_F(StructureUtilsTestFixture, TestCreatePairListSizes) {
        for (TestCaseData &test_case : test_cases) {
            matrix<double> lattice (matrix_from_multi_array(test_case.lattice));
            matrix<double> fcoords (matrix_from_multi_array(test_case.fcoords));
            std::map<shell_t, size_t> counts;
            std::map<shell_t, double> all_weights;
            auto natoms {static_cast<index_t>(fcoords.size1())};
            auto pbc_vecs = sqsgenerator::utils::pbc_shortest_vectors(lattice, fcoords, true);
            auto d2 = sqsgenerator::utils::distance_matrix(pbc_vecs);
            auto distances = sqsgenerator::utils::default_shell_distances(d2);
            pair_shell_matrix_t shells = sqsgenerator::utils::shell_matrix(d2, distances);
            auto max_shell = static_cast<index_t>(*std::max_element(shells.origin(), shells.origin()+shells.num_elements()));
            for (auto i = 1; i <= max_shell; i++) {
                counts.insert(std::make_pair(i, 0));
                all_weights.insert(std::make_pair(i, 0.0));
            }

            // If all shells are present, the list length must be the number of pairs in the structure
            ASSERT_EQ(create_pair_list(shells, all_weights).size(), natoms*(natoms-1)/2);
            for (index_t i = 0; i < natoms; i++) {
                for (index_t j = i+1; j < natoms; j++) {
                    counts[shells[i][j]]++;
                }
            }

            // If there's only one shell it must be exactly the number of pairs in the corresponding shell
            for (auto i = 1; i <= max_shell; i++) {
                auto num_pairs = create_pair_list(shells, {{i, 0.0}}).size();
                auto should_be = counts[i];
                ASSERT_EQ(num_pairs, should_be);
            }
            // Same must be true for any combination of two shells
            for (auto i = 1; i <= max_shell; i++) {
                for (auto j = i+1; j <= max_shell; j++) {
                    auto num_pairs = create_pair_list(shells, {{i, 0.0}, {j, 0.0}}).size();
                    auto should_be = counts[i]+counts[j];
                    ASSERT_EQ(num_pairs, should_be);
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}