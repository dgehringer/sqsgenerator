//
// Created by dominik on 30.05.21.
//

#include "containers.h"
#include <boost/multi_array.hpp>
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>


namespace p = boost::python;
namespace np = boost::python::numpy;

template<typename T>
void printVec(const std::string &name, const std::vector<T> &vec) {
    std::cout << name << " {";
    for (auto &s: vec) {
        std::cout << s << " ";
    }
    std::cout << "}\n";
}

template<typename T>
np::ndarray toFlatNumpyArray(const T *array, size_t num_elements) {
    return np::from_data(array,
                         np::dtype::get_builtin<T>(),
                         p::make_tuple(num_elements),
                         p::make_tuple(sizeof(T)),
                         p::object());
}

template<typename T>
np::ndarray toShapedNumpyArray(const T *array, const std::vector<size_t> &shape) {
    std::vector<size_t> strides;
    for (size_t i = 1; i < shape.size(); i++) {
        size_t nelem {1};
        for (size_t j = 0; j < i; j++) nelem *= shape[j];
        strides.push_back(nelem);
    }
    std::reverse(strides.begin(), strides.end());
    strides.push_back(1);
    std::transform(strides.begin(), strides.end(), strides.begin(), std::bind1st(std::multiplies<size_t>(), sizeof(T)));

    return np::from_data(array,
                         np::dtype::get_builtin<T>(),
                         shape,
                         strides,
                         p::object());
}

template<typename T, size_t...Shape>
np::ndarray toShapedNumpyArray(const T* array) {
    std::vector<size_t> shape {Shape...};
    return toShapedNumpyArray<T>(array, shape);
}



class PairSQSResultPythonWrapper : public PairSQSResult {

    public:
        PairSQSResultPythonWrapper(const PairSQSResult &other) : PairSQSResult(other){}

        np::ndarray getConfiguration() {
            std::vector<size_t> shape {configuration.size()};
            return toShapedNumpyArray<Species>(configuration.data(), shape);
        }

    np::ndarray getParameters() {
        std::vector<size_t> shape {parameters.shape(), parameters.shape()+parameters.num_dimensions()};
        return toShapedNumpyArray<double>(parameters.data(), shape);
    }
    };

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
static PairSQSResultPythonWrapper instance(result);

// https://stackoverflow.com/questions/49692157/boost-python-return-python-object-which-references-to-existing-c-objects
template <typename T>
inline p::object wrapExistingInPythonObject(T ptr) {
    typename p::reference_existing_object::apply<T>::type converter;
    auto converted = converter(ptr);
    p::handle handle(converted);
    return p::object(handle);
}

p::object getData() {
    return wrapExistingInPythonObject<PairSQSResultPythonWrapper&>(instance);
}



BOOST_PYTHON_MODULE(data) {
    Py_Initialize();
    np::initialize();

    p::class_<PairSQSResultPythonWrapper>("PairSQSResult", p::no_init)
            .def_readonly("objective", &PairSQSResultPythonWrapper::objective)
            .def_readonly("rank", &PairSQSResultPythonWrapper::rank)
            .def_readonly("configuration", &PairSQSResultPythonWrapper::getConfiguration)
            .def_readonly("parameters", &PairSQSResultPythonWrapper::getParameters);
    p::def("get_data", getData);
 //boost::python::class_<PairSQSResult>("PairSQSResult", boost::python::init<double, uint64_t, Configuration, PairSROParameters>());
}
