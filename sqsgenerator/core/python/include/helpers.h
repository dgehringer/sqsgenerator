//
// Created by dominik on 30.05.21.
//

#ifndef SQSGENERATOR_HELPERS_H
#define SQSGENERATOR_HELPERS_H


#include <boost/python.hpp>
#include <boost/python/numpy.hpp>

namespace py = boost::python;
namespace np = boost::python::numpy;

namespace sqsgenerator::python::helpers {

            struct Cpp_int_to_python_num {
                static PyObject* convert(const cpp_int& number) {
                    std::ostringstream oss;
                    oss << std::hex << number;
                    return PyLong_FromString(oss.str().c_str(), nullptr, 16);
                }
            };

            struct Cpp_int_from_python_num {

                Cpp_int_from_python_num()
                {
                    py::converter::registry::push_back(
                            &convertible,
                            &construct,
                            py::type_id<cpp_int>());
                }

                static void* convertible(PyObject *obj_ptr) {
                    return PyNumber_Check(obj_ptr) ? obj_ptr : nullptr;
                }

                static void construct(PyObject *obj_ptr, boost::python::converter::rvalue_from_python_stage1_data* data) {
                    py::handle<> handle(py::borrowed(obj_ptr));
                    typedef py::converter::rvalue_from_python_storage<cpp_int> storage_type;
                    void* storage = reinterpret_cast<storage_type*>(data)->storage.bytes;
                    Py_ssize_t size = 0;
                    PyObject* tmp = PyNumber_ToBase(obj_ptr, 16);
                    const char* c_str_ptr = PyUnicode_AsUTF8AndSize(tmp, &size);
                    std::string s(c_str_ptr, size);
                    if (tmp == nullptr) boost::python::throw_error_already_set();

                    new (storage) cpp_int (s);
                    Py_DECREF(tmp);
                    data->convertible = storage;
                }
            };

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
#endif //SQSGENERATOR_HELPERS_H
