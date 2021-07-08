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


template<typename MultiArray>
void print_array(MultiArray mat) {
    auto shape {shape_from_multi_array(mat)};
    auto rows {shape[0]}, cols {shape[1]};
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
                return helpers::to_flat_numpy_array<species_t>(
                        m_handle.configuration().data(),
                        m_handle.configuration().size()
                        );
            }

            np::ndarray parameters(py::tuple const &shape) {
                   return helpers::to_flat_numpy_array<double>(
                           m_handle.storage().data(),
                           m_handle.storage().size()).reshape(shape);
            }

        };

        class StructurePythonWrapper {
        private:
            atomistics::Structure m_handle;
        public:
            StructurePythonWrapper(np::ndarray lattice, np::ndarray frac_coords, py::object symbols)
            : m_handle(
                    helpers::ndarray_to_multi_array<double, 2>(lattice),
                            helpers::ndarray_to_multi_array<double,2>(frac_coords),
                       helpers::list_to_vector<std::string>(symbols),
                               {true,true, true})
            {
            }

            np::ndarray lattice() {
                return helpers::multi_array_to_ndarray<const_array_2d_ref_t,2>(m_handle.lattice());
            }

            np::ndarray frac_coords() {
                return helpers::multi_array_to_ndarray<const_array_2d_ref_t,2>(m_handle.frac_coords());
            }

            py::list species() {
                return helpers::vector_to_list(m_handle.species());
            }

            py::tuple pbc() {
                auto pbc {m_handle.pbc()};
                return py::make_tuple(pbc[0], pbc[1], pbc[2]);
            }

            np::ndarray distance_vecs() {
                return helpers::multi_array_to_ndarray<const_array_3d_ref_t , 3>(m_handle.distance_vecs());
            }

            np::ndarray distance_matrix() {
                return helpers::multi_array_to_ndarray<const_array_2d_ref_t , 2>(m_handle.distance_matrix());
            }

            np::ndarray shell_matrix(uint8_t prec = 5) {
                return helpers::multi_array_to_ndarray<const_pair_shell_matrix_ref_t, 2>(m_handle.shell_matrix(prec));
            }

        };


        typedef std::vector<SQSResultPythonWrapper> SQSResultCollectionPythonWrapper;
    }

using namespace sqsgenerator;

static configuration_t conf {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};
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

    py::class_<StructurePythonWrapper>("Structure", py::init<np::ndarray, np::ndarray, py::object>())
            .def_readonly("lattice", &StructurePythonWrapper::lattice)
            .def_readonly("species", &StructurePythonWrapper::species)
            .def_readonly("frac_coords", &StructurePythonWrapper::frac_coords)
            .def_readonly("distance_vecs", &StructurePythonWrapper::distance_vecs)
            .def_readonly("distance_matrix", &StructurePythonWrapper::distance_matrix)
            .def("shell_matrix", &StructurePythonWrapper::shell_matrix)
            .def_readonly("pbc", &StructurePythonWrapper::pbc);

    py::class_<SQSResultCollectionPythonWrapper>("PairSQSResultCollection")
            .def("__len__", &SQSResultCollectionPythonWrapper::size)
            .def("__getitem__", &helpers::std_item<SQSResultCollectionPythonWrapper>::get,
                 py::return_value_policy<py::copy_non_const_reference>())
            .def("__iter__", py::iterator<SQSResultCollectionPythonWrapper>());
    py::def("get_data", get_data);

}
