//
// Created by dominik on 30.05.21.
//

#include "containers.h"
#include "helpers.h"
#include <boost/multi_array.hpp>
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>


namespace py = boost::python;
namespace np = boost::python::numpy;
namespace helpers = sqsgenerator::python::helpers;


namespace sqsgenerator {

    namespace python {


        class PairSQSResultPythonWrapper : public PairSQSResult {

        public:
            PairSQSResultPythonWrapper(const PairSQSResult &other) : PairSQSResult(other) {}

            np::ndarray getConfiguration() {
                std::vector<size_t> shape{configuration.size()};
                return helpers::toShapedNumpyArray<Species>(configuration.data(), shape);
            }

            np::ndarray getParameters() {
                std::vector<size_t> shape{parameters.shape(), parameters.shape() + parameters.num_dimensions()};
                return helpers::toShapedNumpyArray<double>(parameters.data(), shape);
            }
        };
    }
}
typedef sqsgenerator::python::PairSQSResultPythonWrapper PairSQSResultWrapper;
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
static boost::multi_array_ref<double, 3> sro(&data[0][0][0], boost::extents[3][3][3]);
static PairSQSResult result(0.0, 1, conf, sro);
static sqsgenerator::python::PairSQSResultPythonWrapper instance(result);


py::object getData() {
    return helpers::wrapExistingInPythonObject<PairSQSResultWrapper&>(instance);
}



BOOST_PYTHON_MODULE(data) {
    Py_Initialize();
    np::initialize();

    py::class_<PairSQSResultWrapper>("PairSQSResult", py::no_init)
            .def_readonly("objective", &PairSQSResultWrapper::objective)
            .def_readonly("rank", &PairSQSResultWrapper::rank)
            .def_readonly("configuration", &PairSQSResultWrapper::getConfiguration)
            .def_readonly("parameters", &PairSQSResultWrapper::getParameters);
    py::def("get_data", getData);

}
