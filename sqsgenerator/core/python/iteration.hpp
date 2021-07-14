//
// Created by dominik on 14.07.21.
//

#ifndef SQSGENERATOR_ITERATION_HPP
#define SQSGENERATOR_ITERATION_HPP

#include "data.hpp"
#include "settings.hpp"
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>

namespace py = boost::python;
namespace np = boost::python::numpy;

namespace sqsgenerator::python {

    class IterationSettingsPythonWrapper {
    private:
        sqsgenerator::IterationSettings m_handle;
// IterationSettings(Structure &structure, double target_objective, array_2d_t parameter_weights,  pair_shell_weights_t shell_weights, int iterations, int output_configurations, iteration_mode mode = iteration_mode::random, uint8_t prec = 5)
    public:
        IterationSettingsPythonWrapper(StructurePythonWrapper wrapper, double target_objective, np::ndarray parameter_weights, py::dict shell_weights, int iterations, int output_configurations, iteration_mode iteration_mode, uint8_t prec);
    };
}

#endif //SQSGENERATOR_ITERATION_HPP
