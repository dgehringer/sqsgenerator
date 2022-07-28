
#include "types.hpp"
#include <vector>
#include <fstream>
#include <cassert>
#include <filesystem>
#include <functional>
#include <gtest/gtest.h>
#include <initializer_list>
#include <boost/multi_array.hpp>


namespace fs = std::filesystem;

namespace sqsgenerator::test {

    // Helper class to determine the full number of elements in the
    // multidimensional array
    template<std::size_t... vs>
    struct ArraySize;

    template<std::size_t v, size_t... vs>
    struct ArraySize<v, vs...> {
        static constexpr size_t size = v * ArraySize<vs...>::size;
    };

    template<>
    struct ArraySize<> {
        static constexpr size_t size = 1;
    };

    // Creates your multi_array
    template<typename T, int... dims>
    boost::multi_array<T, sizeof...(dims)> create_multi_array(std::initializer_list<T> l) {
        constexpr size_t asize = ArraySize<dims...>::size;
        assert(l.size() == asize); // could be a static assert in C++14

        // Dump data into a vector (because it has the right kind of ctor)
        const std::vector<T> a(l);
        // This can be used in a multi_array_ref ctor.
        boost::const_multi_array_ref<T, sizeof...(dims)> mar(
                &a[0],
                std::array<int, sizeof...(dims)>{dims...});
        // Finally, deep-copy it into the structure we can return.
        return boost::multi_array<T, sizeof...(dims)>(mar);
    }


    template<typename Return, typename Arg>
    using Transform = std::function<Return(Arg)>;

    template<typename VectorA, typename VectorB, typename C>
    void assert_vector_equals(
            const VectorA &a,
            const VectorB &b,
            Transform<typename VectorA::value_type, C> t1 = std::function<C(typename VectorA::value_type)>([](const typename VectorA::value_type &a) { return a; }),
            Transform<typename VectorB::value_type, C> t2 = std::function<C(typename VectorB::value_type)>([](const typename VectorB::value_type &a) { return a; })) {

        ASSERT_EQ(a.size(), b.size());
        for (auto i = 0; i < a.size(); i++) {
            ASSERT_EQ(t1(a[i]), t2(b[i]));
        }
    }

    template<typename MultiArrayA, typename MultiArrayB, typename C>
    void assert_multi_array_near(
            MultiArrayA a,
            MultiArrayB b,
            Transform<typename MultiArrayA::element, C> t1 = std::function<C(typename MultiArrayA::element)>([](const typename MultiArrayA::element &a) { return a; }),
            Transform<typename MultiArrayB::element, C> t2 = std::function<C(typename MultiArrayB::element)>([](const typename MultiArrayA::element &a) { return a; }),
            C tol = 1.05e-7) {

        ASSERT_EQ(a.num_elements(), b.num_elements());
        auto p1 {a.data()};
        auto p2 {b.data()};

        for (size_t i = 0; i < a.num_elements(); i++) {
            ASSERT_NEAR(t1(p1[i]), t2(p2[i]), tol);
        }
    }


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
    boost::multi_array<T, NDims> read_array(std::ifstream &fhandle, std::string const &name) {
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


        boost::multi_array<T, NDims> result;
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
        configuration_t configuration;
    };

    TestCaseData read_test_data(std::string const &path) {
        std::ifstream fhandle(path);
        std::string line;
        auto lattice = read_array<double, 2>(fhandle, "lattice");
        auto numbers = read_array<species_t, 1>(fhandle, "numbers");
        auto fcoords = read_array<double, 2>(fhandle, "fcoords");
        auto d2 = read_array<double, 2>(fhandle, "distances");
        auto shells = read_array<shell_t, 2>(fhandle, "shells");
        auto vecs = read_array<double, 3>(fhandle, "vecs");

        configuration_t configuration(numbers.begin(), numbers.end());

        fhandle.close();
        return TestCaseData {lattice, fcoords, d2, vecs, shells, configuration};
    }

    std::vector<TestCaseData> load_test_cases(const std::string &folder, const std::string &ext = ".data") {
        std::vector<TestCaseData> test_cases;
        for (auto &p : get_all(folder, ext)) {
            // std::cout << "StructureUtilsTestFixture::SetUp(): Found test case: " << p << std::endl;
            test_cases.emplace_back(read_test_data(p));
        }
        return test_cases;
    }

}