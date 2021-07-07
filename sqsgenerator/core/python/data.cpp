//
// Created by dominik on 30.05.21.
//

#include "helpers.hpp"
#include "atomistics.hpp"
#include "containers.hpp"
#include "container_wrappers.hpp"
#include <boost/python.hpp>
#include <boost/multi_array.hpp>
#include <boost/python/numpy.hpp>
#include <boost/multiprecision/cpp_int.hpp>


namespace py = boost::python;
namespace np = boost::python::numpy;
namespace helpers = sqsgenerator::python::helpers;
namespace atomistics = sqsgenerator::utils::atomistics;


void print_array(const multi_array<double,2> &mat, size_t rows, size_t cols) {
    std::cout << "[";
    for (size_t i = 0; i < rows; i++) {
        std::cout << "[";
        for (size_t j = 0; j < cols - 1; j++) {
            std::cout << mat[i][j] << ", ";
        }
        std::cout << mat[i][cols- 1] << "]";
        if (i < rows - 1) std::cout << std::endl;
    }
    std::cout << "]" << std::endl;
}

namespace sqsgenerator::python {

        class SQSResultPythonWrapper {


        private:
            const SQSResult& m_handle;

        public:

            explicit SQSResultPythonWrapper(const SQSResult &other) : m_handle(other) {
            }

            double objective(){
                return m_handle.objective();
            }

            cpp_int rank() {
                return m_handle.rank();
            }

            np::ndarray configuration() {
                return helpers::to_shaped_numpy_array<Species>(
                        m_handle.configuration().data(),
                        {m_handle.configuration().size()}
                        );
            }

            np::ndarray parameters(py::tuple const &shape) {
                   return helpers::to_shaped_numpy_array<double>(
                           m_handle.storage().data(),
                           {m_handle.storage().size()}).reshape(shape);
            }
        };

        class StructurePythonWrapper {
        private:
            const atomistics::Structure m_handle;
        public:
            StructurePythonWrapper(np::ndarray lattice, np::ndarray frac_coords, py::object symbols)
            : m_handle(
                    helpers::ndarray_to_multi_array<double, 2>(lattice),
                            helpers::ndarray_to_multi_array<double,2>(frac_coords),
                       helpers::py_list_to_std_vector<std::string>(symbols),
                               {true,true, true})
            {}
        };


        typedef std::vector<SQSResultPythonWrapper> SQSResultCollectionPythonWrapper;
    }

using namespace sqsgenerator;

static Configuration conf {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};
static double data[3][3][3] {
        {{0,1,2},
        {3,4,5},
        {6,7,8}},
        {{9,10,11},
         {12,13,14},
         {15,16,17}},
        {{18,19,20},
         {21,22,23},
         {24,25,26}}
};

using namespace sqsgenerator::python;


static ParameterStorage sro_vec {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};
static SQSResultCollection queue(20);
static SQSResultCollectionPythonWrapper results;
static bool resultsInitialized = false;


py::object get_data() {
    cpp_int huge_int = {1000000000000000000};
    if (!resultsInitialized) {
        for (size_t i = 0; i < 20; ++i) {
            SQSResult lresult( static_cast<double>(1),  i*huge_int, conf, sro_vec);
            queue.add_result(lresult);
        }
        std::cout << "Collecting queue" << std::endl;
        queue.collect();
        for (auto &r : queue.results()) results.push_back(SQSResultPythonWrapper(r));
        resultsInitialized = true;
    }

    return helpers::warp_in_existing_python_object<SQSResultCollectionPythonWrapper&>(results);
}


void initialize_converters()
{
    py::to_python_converter<cpp_int, helpers::Cpp_int_to_python_num>();
    helpers::Cpp_int_from_python_num();
}



BOOST_PYTHON_MODULE(data) {
    Py_Initialize();
    np::initialize();
    initialize_converters();


    py::class_<SQSResultPythonWrapper>("PairSQSResult", py::no_init)
            .def_readonly("objective", &SQSResultPythonWrapper::objective)
            .def_readonly("rank", &SQSResultPythonWrapper::rank)
            .def_readonly("configuration", &SQSResultPythonWrapper::configuration)
            .def("parameters", &SQSResultPythonWrapper::parameters);


    py::class_<atomistics::Atom>("Atom", py::no_init)
            .def_readonly("Z", &atomistics::Atom::Z)
            .def_readonly("name", &atomistics::Atom::name)
            .def_readonly("symbol", &atomistics::Atom::symbol)
            .def_readonly("atomic_radius", &atomistics::Atom::atomic_radius)
            .def_readonly("mass", &atomistics::Atom::mass);

    py::class_<StructurePythonWrapper>("Structure", py::init<np::ndarray, np::ndarray, py::object>());

    py::class_<SQSResultCollectionPythonWrapper>("PairSQSResultCollection")
            .def("__len__", &SQSResultCollectionPythonWrapper::size)
            .def("__getitem__", &helpers::std_item<SQSResultCollectionPythonWrapper>::get,
                 py::return_value_policy<py::copy_non_const_reference>())
            .def("__iter__", py::iterator<SQSResultCollectionPythonWrapper>());
    py::def("get_data", get_data);

}
