//
// Created by dominik on 29.06.21.
//
#include "types.hpp"
#include "utils.hpp"
#include "structure_utils.hpp"
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <gtest/gtest.h>

using namespace boost;
using namespace sqsgenerator::utils;
using namespace boost::numeric::ublas;
namespace fs = std::filesystem;

namespace sqsgenerator::test {

    template <typename T> T convert_to (const std::string &str)
    {
        std::istringstream ss(str);
        T num;
        ss >> num;
        return num;
    }
/**
 * \brief   Return the filenames of all files that have the specified extension
 *          in the specified directory and all subdirectories.
 */
    std::vector<fs::path> get_all(std::string const &root, std::string const &ext) {
        std::vector<fs::path> paths;
        for (auto &p : fs::recursive_directory_iterator(root))
        {
            if (p.path().extension() == ext) paths.emplace_back(p.path());
        }
        return paths;
    }

    std::vector<std::string> split (std::string s, std::string delimiter) {
        // Taken from https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }

        res.push_back (s.substr (pos_start));
        return res;
    }

    template<typename T, size_t NDims>
    multi_array<T, NDims> read_array(std::ifstream &fhandle, std::string const &name) {
        std::string line;
        std::string start_line = name + "::array::begin";
        while (std::getline(fhandle, line)) {
            if (line == start_line) { break; }
        }
        assert(line == start_line);
        // Read dimensions
        std::getline(fhandle, line);
        auto crumbs  = split(line, " ");
        assert(crumbs.size() == 2);
        assert(crumbs[0] == name + "::array::ndims");
        size_t ndims {std::stoul(crumbs[1])};
        assert(ndims == NDims);

        // Read shape
        std::getline(fhandle, line);
        crumbs  = split(line, " ");
        assert(crumbs.size() == ndims+1);
        assert(crumbs[0] == name + "::array::shape");
        std::vector<size_t> shape;
        for(auto it = crumbs.begin() + 1; it != crumbs.end(); ++it) shape.push_back(std::stoul(*it));
        assert(shape.size() == ndims);
        size_t num_elements = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

        std::getline(fhandle, line);
        crumbs  = split(line, " ");
        assert(crumbs.size() == num_elements+1);
        assert(crumbs[0] == name + "::array::data");
        std::vector<T> data;
        for(auto it = crumbs.begin() + 1; it != crumbs.end(); ++it) data.push_back(convert_to<T>(*it));
        assert(data.size() == num_elements);


        multi_array<T, NDims> result;
        auto& shape_array = reinterpret_cast<boost::array<size_t, NDims> const&>(*shape.data());
        result.resize(shape_array);
        result.assign(data.begin(), data.end());
        std::getline(fhandle, line);
        assert(line == name + "::array::end");
        return result;
    }

    struct TestCaseData {
    public:
        array_2d_t lattice;
        array_2d_t fcoords;
        array_2d_t distances;
        array_3d_t vecs;
        pair_shell_matrix_t shells;
    };

    TestCaseData read_test_data(std::string const &path) {
        std::ifstream fhandle(path);
        std::string line;
        auto lattice = read_array<double, 2>(fhandle, "lattice");
        auto fcoords = read_array<double, 2>(fhandle, "fcoords");
        auto d2 = read_array<double, 2>(fhandle, "distances");
        auto shells = read_array<shell_t, 2>(fhandle, "shells");
        auto vecs = read_array<double, 3>(fhandle, "vecs");

        fhandle.close();
        return TestCaseData {lattice, fcoords, d2, vecs, shells};
    }

    class StructureUtilsTestFixture : public ::testing::Test {
    protected:
        std::vector<TestCaseData> test_cases{};


    public:
        void SetUp() {
            // code here will execute just before the test ensues
            // std::cout << "StructureUtilsTestFixture::SetUp(): " << std::filesystem::current_path() << std::endl;
            get_all("resources", ".data");
            for (auto &p : get_all("resources", ".data")) {
                // std::cout << "StructureUtilsTestFixture::SetUp(): Found test case: " << p << std::endl;
                test_cases.emplace_back(read_test_data(p));
            }
        };

        void TearDown() {
            // code here will be called just after the test completes
            // ok to through exceptions from here if need be
        }


    };

    template<typename MultiArrayA>
    void assert_multi_array_equal(const MultiArrayA &a, const MultiArrayA &b) {
        typedef typename MultiArrayA::element T;

        ASSERT_EQ(a.num_elements(), b.num_elements());
        for (size_t i = 0; i < a.num_elements(); ++i) {
            ASSERT_NEAR(std::abs<T>(a.data()[i]), std::abs<T>(b.data()[i]), 1.0e-5);
            //EXPECT_NEAR(a.data()[i], b.data()[i], 1.0e-5);
        }
    }

    template<>
    void assert_multi_array_equal<pair_shell_matrix_t>(const pair_shell_matrix_t &a, const pair_shell_matrix_t &b) {
        ASSERT_EQ(a.num_elements(), b.num_elements());
        for (size_t i = 0; i < a.num_elements(); ++i) {
            //ASSERT_NEAR(std::abs<int>(a.data()[i]), std::abs<int>(b.data()[i]), 1.0e-5);
            EXPECT_NEAR(std::abs<int>(a.data()[i]), std::abs<int>(b.data()[i]), 1.0e-5);
            //EXPECT_NEAR(a.data()[i], b.data()[i], 1.0e-5);
        }
    }

    TEST_F(StructureUtilsTestFixture, TestPbcVectors) {
        for (TestCaseData &test_case : test_cases) {
            matrix<double> lattice (matrix_from_multi_array(test_case.lattice));
            matrix<double> fcoords (matrix_from_multi_array(test_case.fcoords));
            auto pbc_vecs = sqsgenerator::utils::pbc_shortest_vectors(lattice, fcoords, true);
            for (size_t i = 0; i < 3; i++) ASSERT_EQ(pbc_vecs.shape()[i], test_case.vecs.shape()[i]);
            assert_multi_array_equal(pbc_vecs, test_case.vecs);
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
            assert_multi_array_equal(d2, test_case.distances);
            auto d2_external = sqsgenerator::utils::distance_matrix(test_case.vecs);
            assert_multi_array_equal(d2, d2_external);
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
            assert_multi_array_equal(shells, test_case.shells);
            /*for (index_t i = 0; i < natoms; i++) {
                for (index_t j = i+1; j < natoms; j++) {
                    if (shells[i][j] != test_case.shells[i][j]) {
                        std::cout << "Different shell (" << i << ", " << j << ") = (" << static_cast<int>(shells[i][j]) << " != " << static_cast<int>(test_case.shells[i][j]) << ") = (" << d2[i][j] << ", " << distances[shells[i][j]] << ")" << std::endl;
                    } else {
                        ASSERT_EQ(shells[i][j], test_case.shells[i][j]);
                    }
                }
            }*/
            auto shells_external = sqsgenerator::utils::shell_matrix(test_case.distances, distances);
            assert_multi_array_equal(shells, shells_external);
            std::cout << format_vector(distances);
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
            // If theres only one shell it must be exactly the number of pairs in the corresponding shell
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