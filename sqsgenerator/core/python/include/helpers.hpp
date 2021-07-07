//
// Created by dominik on 30.05.21.
//

#ifndef SQSGENERATOR_HELPERS_H
#define SQSGENERATOR_HELPERS_H

#include "utils.hpp"
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>
#include <boost/multi_array.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace py = boost::python;
namespace np = boost::python::numpy;

using namespace boost::multiprecision;

namespace sqsgenerator::python::helpers {

    struct Cpp_int_to_python_num {
        static PyObject *convert(const cpp_int &number) {
            std::ostringstream oss;
            oss << std::hex << number;
            return PyLong_FromString(oss.str().c_str(), nullptr, 16);
        }
    };

    struct Cpp_int_from_python_num {

        Cpp_int_from_python_num() {
            py::converter::registry::push_back(
                    &convertible,
                    &construct,
                    py::type_id<cpp_int>());
        }

        static void *convertible(PyObject *obj_ptr) {
            return PyNumber_Check(obj_ptr) ? obj_ptr : nullptr;
        }

        static void construct(PyObject *obj_ptr, boost::python::converter::rvalue_from_python_stage1_data *data) {
            py::handle<> handle(py::borrowed(obj_ptr));
            typedef py::converter::rvalue_from_python_storage<cpp_int> storage_type;
            void *storage = reinterpret_cast<storage_type *>(data)->storage.bytes;
            Py_ssize_t size = 0;
            PyObject * tmp = PyNumber_ToBase(obj_ptr, 16);
            const char *c_str_ptr = PyUnicode_AsUTF8AndSize(tmp, &size);
            std::string s(c_str_ptr, size);
            if (tmp == nullptr) boost::python::throw_error_already_set();

            new(storage) cpp_int(s);
            Py_DECREF(tmp);
            data->convertible = storage;
        }
    };

    template<typename T>
    np::ndarray to_flat_numpy_array(const T *array, size_t num_elements) {
        return np::from_data(array,
                             np::dtype::get_builtin<T>(),
                             py::make_tuple(num_elements),
                             py::make_tuple(sizeof(T)),
                             py::object());
    }

    template<typename T>
    np::ndarray to_shaped_numpy_array(const T *array, const std::vector<size_t> &shape) {
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
    np::ndarray to_shaped_numpy_array(const T *array) {
        std::vector<size_t> shape{Shape...};
        return to_shaped_numpy_array<T>(array, shape);
    }


    template<typename T, size_t NDims>
    boost::multi_array<T, NDims> ndarray_to_multi_array(np::ndarray &array) {
        assert(array.get_nd() == NDims);
        std::vector<size_t> shape(array.get_shape(), array.get_shape()+array.get_nd());
        return boost::multi_array<T, NDims>(
                boost::multi_array_ref<T, NDims>(
                        reinterpret_cast<T*>(array.astype(np::dtype::get_builtin<T>()).get_data()), shape
                        )
                );
    }

    template<typename MultiArray>
    np::ndarray multi_array_to_ndarray(const MultiArray &array){
        typedef typename MultiArray::element T;
        auto shape {shape_from_multi_array(array)};
        return to_shaped_numpy_array(array.data(), shape);

    }


    // https://stackoverflow.com/questions/49692157/boost-python-return-python-object-which-references-to-existing-c-objects
    template<typename T>
    inline py::object warp_in_existing_python_object(T obj) {
        typename py::reference_existing_object::apply<T>::type converter;
        auto converted = converter(obj);
        py::handle handle(converted);
        return py::object(handle);
    }

    template<typename T>
    inline
    std::vector<T> list_to_vector(const py::object &iterable) {
        return std::vector<T>(py::stl_input_iterator<T>(iterable),
                              py::stl_input_iterator<T>());
    }


    template<class T>
    inline
    py::list vector_to_list(std::vector<T> vector) {
        typename std::vector<T>::iterator iter;
        py::list list;
        for (iter = vector.begin(); iter != vector.end(); ++iter) {
            list.append(*iter);
        }
        return list;
    }
}
#endif //SQSGENERATOR_HELPERS_H
