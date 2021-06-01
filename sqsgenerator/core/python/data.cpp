//
// Created by dominik on 30.05.21.
//

#include "containers.h"
#include "helpers.h"
#include "container_wrappers.h"
#include <boost/multi_array.hpp>
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>


namespace py = boost::python;
namespace np = boost::python::numpy;
namespace helpers = sqsgenerator::python::helpers;


namespace sqsgenerator {

    namespace python {

        void IndexError() { PyErr_SetString(PyExc_IndexError, "Index out of range"); }

        template<typename T>
        class SQSResultPythonWrapper {
        private:
            const SQSResult<T>& m_handle;

        public:

            SQSResultPythonWrapper(const SQSResult<T> &other) : m_handle(other) {
            }

            double objective(){
                return m_handle.objective;
            }

            cpp_int rank() {
                return m_handle.rank;
            }

            np::ndarray configuration() {
                std::vector<size_t> shape{m_handle.configuration.size()};
                return helpers::toShapedNumpyArray<Species>(m_handle.configuration.data(), shape);
            }

            np::ndarray parameters() {
                std::vector<size_t> shape{m_handle.parameters.shape(), m_handle.parameters.shape() + m_handle.parameters.num_dimensions()};
                return helpers::toShapedNumpyArray<double>(m_handle.parameters.data(), shape);
            }
        };

        template<typename T>
        using SQSResultCollectionPythonWrapper = std::vector<SQSResultPythonWrapper<T>>;

        typedef SQSResultPythonWrapper<PairSROParameters> PairSQSResultPythonWrapper;
        typedef SQSResultPythonWrapper<TripletSROParameters> TripletSQSResultPythonWrapper;
        typedef SQSResultCollectionPythonWrapper<PairSROParameters> PairSQSResultCollectionPythonWrapper;
        typedef SQSResultCollectionPythonWrapper<TripletSROParameters> TripletSQSResultCollectionPythonWrapper;
    }
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

static boost::multi_array_ref<double, 3> sro(&data[0][0][0], boost::extents[3][3][3]);
static PairSQSResult result(0.0, 1, conf, sro);
static PairSQSIterationResult queue(20);
static PairSQSResultCollectionPythonWrapper results;
static bool resultsInitialized = false;


py::object getData() {
    if (!resultsInitialized) {
        for (int i = 0; i < 20; ++i) {
            result.rank = i;
            queue.addResult(result);
        }
        std::cout << "Collecting queue" << std::endl;
        queue.collect();
        for (auto &r : queue.results()) results.push_back(PairSQSResultPythonWrapper(r));
        resultsInitialized = true;
    }

    return helpers::wrapExistingInPythonObject<PairSQSResultCollectionPythonWrapper&>(results);
}


BOOST_PYTHON_MODULE(data) {
    Py_Initialize();
    np::initialize();

    py::class_<PairSQSResultPythonWrapper>("PairSQSResult", py::no_init)
            .def_readonly("objective", &PairSQSResultPythonWrapper::objective)
            .def_readonly("rank", &PairSQSResultPythonWrapper::rank)
            .def_readonly("configuration", &PairSQSResultPythonWrapper::configuration)
            .def_readonly("parameters", &PairSQSResultPythonWrapper::parameters);

    py::class_<PairSQSResultCollectionPythonWrapper>("PairSQSResultCollection")
            .def("__len__", &PairSQSResultCollectionPythonWrapper::size)
            .def("__getitem__", &helpers::std_item<PairSQSResultCollectionPythonWrapper>::get,
                 py::return_value_policy<py::copy_non_const_reference>())
            .def("__iter__", py::iterator<PairSQSResultCollectionPythonWrapper>());
    py::def("get_data", getData);

}
