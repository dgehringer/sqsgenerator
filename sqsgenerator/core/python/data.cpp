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

template<typename T, size_t... Sizes>
np::ndarray toShapedNumpyArray(const T *array) {
    std::vector<size_t> shape {Sizes...};
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



np::ndarray hello1() {
    size_t size = 125;
    double* data = (double *) malloc(size* sizeof(double));
    size_t cnt = 0;
    for (int i = 0; i < size; i++) {
        data[i] = static_cast<double>(i);
    }
    return toShapedNumpyArray<double, 5, 5, 5>(data);
}
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

np::ndarray getData() {
    return toShapedNumpyArray<double, 3,3,3>(result.parameters.data());
}

BOOST_PYTHON_MODULE(data) {
    Py_Initialize();
    np::initialize();

    boost::python::def("hello", hello1);
    boost::python::def("get_data", getData);
 //boost::python::class_<PairSQSResult>("PairSQSResult", boost::python::init<double, uint64_t, Configuration, PairSROParameters>());
}
