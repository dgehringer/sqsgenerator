//
// Created by dominik on 14.07.21.
//

#ifndef SQSGENERATOR_ITERATION_HPP
#define SQSGENERATOR_ITERATION_HPP

#include "data.hpp"
#include "settings.hpp"
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>

using namespace sqsgenerator;
namespace py = boost::python;
namespace np = boost::python::numpy;

namespace sqsgenerator::python {

    class IterationSettingsPythonWrapper {
    private:
        StructurePythonWrapper m_structure;
        std::shared_ptr<IterationSettings> m_handle;
// IterationSettings(Structure &structure, double target_objective, array_2d_t parameter_weights,  pair_shell_weights_t shell_weights, int iterations, int output_configurations, iteration_mode mode = iteration_mode::random, uint8_t prec = 5)
    public:
        IterationSettingsPythonWrapper(StructurePythonWrapper structure, np::ndarray target_objective, np::ndarray parameter_weights, py::dict shell_weights, int iterations, int output_configurations, iteration_mode iteration_mode);
        IterationSettingsPythonWrapper(StructurePythonWrapper structure, np::ndarray target_objective, np::ndarray parameter_weights, py::dict shell_weights, int iterations, int output_configurations, double atol, double rtol, iteration_mode iteration_mode);
        IterationSettingsPythonWrapper(StructurePythonWrapper structure, np::ndarray target_objective, np::ndarray parameter_weights, py::dict shell_weights, int iterations, int output_configurations, py::list distances, double atol, double rtol, iteration_mode iteration_mode);
        IterationSettingsPythonWrapper(const IterationSettingsPythonWrapper &other) = default;
        double atol() const;
        double rtol() const;
        size_t num_atoms() const;
        size_t num_shells() const;
        int num_iterations() const;
        size_t num_species() const;
        iteration_mode mode() const;
        py::dict shell_weights() const;
        np::ndarray target_objective() const;
        int num_output_configurations() const;
        np::ndarray parameter_weights() const;
        StructurePythonWrapper structure() const;
        std::shared_ptr<IterationSettings> handle() const;
    };
}

#endif //SQSGENERATOR_ITERATION_HPP
