//
// Created by dominik on 14.07.21.
//

#include "iteration.hpp"
#include "helpers.hpp"

namespace sqsgenerator::python {


    IterationSettingsPythonWrapper::IterationSettingsPythonWrapper(StructurePythonWrapper structure, np::ndarray target_objective, np::ndarray parameter_weights, py::dict shell_weights, int iterations, int output_configurations, iteration_mode iteration_mode, uint8_t prec):
    m_structure(structure),
    m_handle(new IterationSettings(
                *structure.handle(),
                helpers::ndarray_to_multi_array<const_array_3d_ref_t>(target_objective),
                helpers::ndarray_to_multi_array<const_array_2d_ref_t>(parameter_weights),
                helpers::dict_to_map<shell_t, double>(shell_weights),
                iterations,
                output_configurations,
                iteration_mode,
                prec
            )) {}

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
