//
// Created by dominik on 30.05.21.
//

#ifndef SQSGENERATOR_HELPERS_H
#define SQSGENERATOR_HELPERS_H

#include <boost/python.hpp>
#include <boost/python/numpy.hpp>

namespace py = boost::python;
namespace np = boost::python::numpy;

namespace sqsgenerator {

    namespace python {

        namespace helpers {
            template<typename T>
            np::ndarray toFlatNumpyArray(const T *array, size_t num_elements) {
                return np::from_data(array,
                                     np::dtype::get_builtin<T>(),
                                     py::make_tuple(num_elements),
                                     py::make_tuple(sizeof(T)),
                                     py::object());
            }

            template<typename T>
            np::ndarray toShapedNumpyArray(const T *array, const std::vector<size_t> &shape) {
                std::vector<size_t> strides;
                for (size_t i = 1; i < shape.size(); i++) {
                    size_t nelem{1};
                    for (size_t j = 0; j < i; j++) nelem *= shape[j];
                    strides.push_back(nelem);
                }
                std::reverse(strides.begin(), strides.end());
                strides.push_back(1);
                std::transform(strides.begin(), strides.end(), strides.begin(),
                               std::bind1st(std::multiplies<size_t>(), sizeof(T)));

                return np::from_data(array,
                                     np::dtype::get_builtin<T>(),
                                     shape,
                                     strides,
                                     py::object());
            }

            template<typename T, size_t...Shape>
            np::ndarray toShapedNumpyArray(const T *array) {
                std::vector<size_t> shape{Shape...};
                return toShapedNumpyArray<T>(array, shape);
            }

            // https://stackoverflow.com/questions/49692157/boost-python-return-python-object-which-references-to-existing-c-objects
            template<typename T>
            inline py::object wrapExistingInPythonObject(T obj) {
                typename py::reference_existing_object::apply<T>::type converter;
                auto converted = converter(obj);
                py::handle handle(converted);
                return py::object(handle);
            }
        }
    }

}
#endif //SQSGENERATOR_HELPERS_H
