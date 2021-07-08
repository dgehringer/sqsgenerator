//
// Created by dominik on 30.05.21.
//

#ifndef SQSGENERATOR_HELPERS_HPP
#define SQSGENERATOR_HELPERS_HPP

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


    template<typename T, size_t NDims>
    boost::multi_array<T, NDims> ndarray_to_multi_array(np::ndarray &array) {
        assert(array.get_nd() == NDims);
        std::vector<size_t> shape(array.get_shape(), array.get_shape()+array.get_nd());
        return boost::multi_array<T, NDims>(
                boost::multi_array_ref<T, NDims>(
                        reinterpret_cast<T*>(array.get_data()), shape
                        )
                );
    }

    // this snippet is an adapted version of:
    // https://stackoverflow.com/questions/28410697/c-convert-vector-to-tuple
    template <typename T, std::size_t... Indices>
    auto vector_to_py_tuple_helper(const std::vector<T>& v, std::index_sequence<Indices...>) {
        return py::make_tuple(v[Indices]...);
    }

    // this snippet is an adapted version of:
    // https://stackoverflow.com/questions/28410697/c-convert-vector-to-tuple
    template <std::size_t N, typename T>
    auto vector_to_py_tuple(const std::vector<T>& v) {
        assert(v.size() >= N);
        return vector_to_py_tuple_helper(v, std::make_index_sequence<N>());
    }

    template<typename MultiArray, size_t NumDims>
    np::ndarray multi_array_to_ndarray(const MultiArray &array){
        typedef typename MultiArray::element T;
        auto shape {vector_to_py_tuple<NumDims, typename MultiArray::size_type>(shape_from_multi_array(array))};
        return to_flat_numpy_array(array.data(), array.num_elements()).reshape(shape);
    }

    // this snippet is taken from:
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
#endif //SQSGENERATOR_HELPERS_HPP