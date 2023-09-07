//
// Created by dominik on 29.09.21.
//

#include "helpers.hpp"
#include "data.hpp"

namespace sqsgenerator::python::helpers {

    composition_t convert_composition(const py::dict &composition) {
        py::list keys = composition.keys();
        composition_t result;
        for (auto i = 0; i < py::len(keys); i++) {
            auto distributed_species{py::extract<species_t>(keys[i])};
            py::dict sublattice_spec = py::extract<py::dict>(composition[keys[i]]);
            py::list sublattice_spec_keys = sublattice_spec.keys();
            std::map<species_t, int> sublattice_spec_internal;

            for (auto j = 0; j < py::len(sublattice_spec_keys); j++) {
                auto destination_lattice{py::extract<species_t>(sublattice_spec_keys[j])};
                auto distributed_num_atoms{py::extract<int>(sublattice_spec[sublattice_spec_keys[j]])};
                sublattice_spec_internal.emplace(destination_lattice, distributed_num_atoms);
            }

            result.emplace(distributed_species, sublattice_spec_internal);
        }
        return result;
    }

    callback_t create_invocation_function(const py::object &callable) {
        return [&callable](rank_t iteration, const SQSResult &result, int mpi_rank, int thread_id) -> bool{
            // we have to wrap the SQSResult object into a python wrapper at first
            
            sqsgenerator::python::SQSResultPythonWrapper wrapped_result(result);
            auto return_value = py::call<PyObject*, rank_t, SQSResultPythonWrapper, int, int>(callable.ptr(), iteration, wrapped_result, mpi_rank, thread_id);
            if (Py_IsNone(return_value)) {
                // a None or no return value means we do not stop the main optimization loop
                return false;
            } else {
                return Py_IsTrue(return_value);
            }
        };
    }

    sqsgenerator::callback_map_t convert_callbacks(const py::dict &callbacks){
        py::list keys = callbacks.keys();

        callback_map_t result;
        for (auto i = 0; i < py::len(keys); i++) {
            auto cb_name {py::extract<std::string>(keys[i])};
            if (DEFAULT_CALLBACKS.find(cb_name) == DEFAULT_CALLBACKS.end()) {
                throw std::invalid_argument("invalid callback name");
            }
            py::list cbs = py::extract<py::list>(callbacks[keys[i]]);
            std::vector<callback_t> converted_callbacks(py::len(cbs));
            for (auto j = 0; j < py::len(cbs); j++) {
                converted_callbacks.push_back(create_invocation_function(py::extract<py::object>(cbs[j])));
            }
            result.emplace(cb_name, converted_callbacks);
        }
        return result;
    }
}

