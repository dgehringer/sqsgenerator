//
// Created by dominik on 14.07.21.
//

#include "iteration.hpp"
#include "helpers.hpp"

namespace sqsgenerator::python {

    shuffling_bounds_t convert_bound_list(py::list &list) {
        shuffling_bounds_t bounds;
        if (list.is_none()) return bounds;
        for (int i = 0; i < len(list); i++) {
            auto bound = list[i];
            bounds.push_back(std::forward_as_tuple(py::extract<size_t>(bound[0]), py::extract<size_t>(bound[1])));
        }
        return bounds;
    }

    composition_t convert_composition(const py::dict &composition) {
        py::list keys = composition.keys();
        composition_t result;
        for (auto i = 0; i < py::len(keys); i++) {
            auto distributed_species { py::extract<species_t>(keys[i]) };
            py::dict sublattice_spec = py::extract<py::dict>(composition[keys[i]]);
            py::list sublattice_spec_keys = sublattice_spec.keys();
            std::map<species_t, int> sublattice_spec_internal;

            for (auto j = 0; j < py::len(sublattice_spec_keys); j++) {
                auto destination_lattice { py::extract<species_t>(sublattice_spec_keys[j]) };
                auto distributed_num_atoms {py::extract<int>(sublattice_spec[sublattice_spec_keys[j]])};
                sublattice_spec_internal.emplace(destination_lattice, distributed_num_atoms);
            }

            result.emplace(distributed_species, sublattice_spec_internal);
        }

        return result;
    }


    IterationSettingsPythonWrapper::IterationSettingsPythonWrapper(
            StructurePythonWrapper structure,
            py::dict composition,
            np::ndarray target_objective,
            np::ndarray parameter_weights,
            py::dict shell_weights,
            int iterations,
            int output_configurations,
            py::list threads_per_rank,
            double atol,
            double rtol,
            iteration_mode iteration_mode) :
    m_structure(structure),
    m_handle(new IterationSettings(
            *structure.handle(),
            convert_composition(composition),
            helpers::ndarray_to_multi_array<const_array_3d_ref_t>(target_objective),
            helpers::ndarray_to_multi_array<const_array_3d_ref_t>(parameter_weights),
            helpers::dict_to_map<shell_t, double>(shell_weights),
            iterations,
            output_configurations,
            helpers::list_to_vector<int>(threads_per_rank),
            atol,
            rtol,
            iteration_mode
    )) {}

    IterationSettingsPythonWrapper::IterationSettingsPythonWrapper(
            StructurePythonWrapper structure,
            py::dict composition,
            np::ndarray target_objective,
            np::ndarray parameter_weights,
            py::dict shell_weights,
            int iterations,
            int output_configurations,
            py::list distances,
            py::list threads_per_rank,
            double atol,
            double rtol,
            iteration_mode iteration_mode) :
    m_structure(structure),
    m_handle(new IterationSettings(
            *structure.handle(),
            convert_composition(composition),
            helpers::ndarray_to_multi_array<const_array_3d_ref_t>(target_objective),
            helpers::ndarray_to_multi_array<const_array_3d_ref_t>(parameter_weights),
            helpers::dict_to_map<shell_t, double>(shell_weights),
            iterations,
            output_configurations,
            helpers::list_to_vector<double>(distances),
            helpers::list_to_vector<int>(threads_per_rank),
            atol,
            rtol,
            iteration_mode
    )) {}

    double IterationSettingsPythonWrapper::atol() const {
        return m_handle->atol();
    }

    double IterationSettingsPythonWrapper::rtol() const {
        return m_handle->rtol();
    }

    size_t IterationSettingsPythonWrapper::num_atoms() const {
        return m_handle->num_atoms();
    }

    size_t IterationSettingsPythonWrapper::num_shells() const {
        return m_handle->num_shells();
    }

    int IterationSettingsPythonWrapper::num_iterations() const {
        return m_handle->num_iterations();
    }

    size_t IterationSettingsPythonWrapper::num_species() const {
        return m_handle->num_species();
    }

    iteration_mode IterationSettingsPythonWrapper::mode() const {
        return m_handle->mode();
    }

    np::ndarray IterationSettingsPythonWrapper::target_objective() const {
        return helpers::multi_array_to_ndarray(m_handle->target_objective());
    }

    StructurePythonWrapper IterationSettingsPythonWrapper::structure() const {
        return m_structure;
    }

    int IterationSettingsPythonWrapper::num_output_configurations() const {
        return m_handle->num_output_configurations();
    }

    py::dict IterationSettingsPythonWrapper::shell_weights() const {
        return helpers::map_to_dict(m_handle->shell_weights());
    }

    np::ndarray IterationSettingsPythonWrapper::parameter_weights() const {
        return helpers::multi_array_to_ndarray(m_handle->parameter_weights());
    }

    std::shared_ptr<IterationSettings> IterationSettingsPythonWrapper::handle() const {
        return m_handle;
    }
}
