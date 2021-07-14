//
// Created by dominik on 30.05.21.
//

#include "data.hpp"
#include "container_wrappers.hpp"

using namespace sqsgenerator;
using namespace sqsgenerator::python;
namespace py = boost::python;
namespace np = boost::python::numpy;
namespace helpers = sqsgenerator::python::helpers;
namespace atomistics = sqsgenerator::utils::atomistics;



void initialize_converters()
{
    py::to_python_converter<cpp_int, helpers::Cpp_int_to_python_num>();
    helpers::Cpp_int_from_python_num();
}



BOOST_PYTHON_MODULE(data) {
    Py_Initialize();
    np::initialize();
    initialize_converters();


    py::class_<SQSResultPythonWrapper>("SQSResult", py::no_init)
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

    py::class_<StructurePythonWrapper>("Structure", py::init<np::ndarray, np::ndarray, py::object, py::tuple>())
            .def_readonly("lattice", &StructurePythonWrapper::lattice)
            .def_readonly("num_atoms", &StructurePythonWrapper::num_atoms)
            .def_readonly("species", &StructurePythonWrapper::species)
            .def_readonly("frac_coords", &StructurePythonWrapper::frac_coords)
            .def_readonly("distance_vecs", &StructurePythonWrapper::distance_vecs)
            .def_readonly("distance_matrix", &StructurePythonWrapper::distance_matrix)
            .def("shell_matrix", &StructurePythonWrapper::shell_matrix)
            .def_readonly("pbc", &StructurePythonWrapper::pbc);

    py::class_<SQSResultCollection>("SQSResultCollection")
            .def("__len__", &SQSResultCollection::size)
            .def("__getitem__", &helpers::std_item<SQSResultCollection>::get,
                 py::return_value_policy<py::copy_non_const_reference>())
            .def("__iter__", py::iterator<SQSResultCollection>());

}
