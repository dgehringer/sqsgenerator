//
// Created by dominik on 14.07.21.
//

#include "iteration.hpp"
#include "helpers.hpp"

namespace sqsgenerator::python {


    IterationSettingsPythonWrapper::IterationSettingsPythonWrapper(StructurePythonWrapper wrapper, double target_objective, np::ndarray parameter_weights, py::dict shell_weights, int iterations, int output_configurations, iteration_mode iteration_mode, uint8_t prec) :
            m_handle(
                    wrapper.handle(),
                    target_objective,
                    helpers::ndarray_to_multi_array<double, 2>(parameter_weights),
                    helpers::dict_to_map<shell_t, double>(shell_weights),
                    iterations,
                    output_configurations,
                    iteration_mode,
                    prec
            ) {}
}
