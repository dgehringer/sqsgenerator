//
// Created by dominik on 30.05.21.
//

#ifndef SQSGENERATOR_HELPERS_HPP
#define SQSGENERATOR_HELPERS_HPP

#include "utils.hpp"
#include <map>
#include <boost/python.hpp>
#include <boost/multi_array.hpp>
#include <boost/python/numpy.hpp>
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


    template<typename MultiArray>
    MultiArray ndarray_to_multi_array(np::ndarray array) {
        auto NDims = MultiArray::dimensionality;
        typedef typename MultiArray::element T;
        assert(array.get_nd() == static_cast<int>(NDims));
        std::vector<size_t> shape(array.get_shape(), array.get_shape()+array.get_nd());
        return MultiArray(reinterpret_cast<T*>(array.get_data()), shape);
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

    template<typename MultiArray>
    np::ndarray multi_array_to_ndarray(const MultiArray &array){
        auto shape {vector_to_py_tuple<MultiArray::dimensionality, typename MultiArray::size_type>(shape_from_multi_array(array))};
        return to_flat_numpy_array(array.data(), array.num_elements()).reshape(shape).copy();
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

    template<typename K, typename V>
    inline
    std::map<K,V> dict_to_map(py::dict d) {
        std::map<K,V> result;
        py::list keys = d.keys();
        auto length {py::len(d)};
        for (ssize_t i = 0; i < length ; i++) {
            K key = py::extract<K>(keys[i]);
            V val = py::extract<V>(d[key]);
            result.emplace(std::make_pair(key, val));
        }
        return result;
    }

    template <class K, class V>
    inline
    boost::python::dict map_to_dict(std::map<K, V> map) {
        typename std::map<K, V>::iterator iter;
        boost::python::dict dictionary;
        for (iter = map.begin(); iter != map.end(); ++iter) {
            dictionary[iter->first] = iter->second;
        }
        return dictionary;
    }
}
#endif //SQSGENERATOR_HELPERS_HPP
