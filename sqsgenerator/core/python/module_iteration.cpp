//
// Created by dominik on 14.07.21.
//
#include "version.hpp"
#include "types.hpp"
#include "sqs.hpp"
#include "data.hpp"
#include "helpers.hpp"
#include "iteration.hpp"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>

namespace logging = boost::log;
using namespace sqsgenerator;
using namespace sqsgenerator::python;

static bool log_initialized = false;

void init_logging() {
    if (! log_initialized) {
        static const std::string COMMON_FMT("[%TimeStamp%][%Severity%]:%Message%");
        boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
        boost::log::add_console_log(
                std::cout,
                boost::log::keywords::format = COMMON_FMT,
                boost::log::keywords::auto_flush = true
        );

        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::trace);
        log_initialized = true;
    }
}

py::tuple pair_sqs_iteration(IterationSettingsPythonWrapper settings) {
    auto [sqs_results, thread_timings] = sqsgenerator::do_pair_iterations(*settings.handle());
    SQSResultCollection wrapped_results;
    for (auto &r : sqs_results) wrapped_results.push_back(SQSResultPythonWrapper(std::move(r)));
    py::dict thread_timings_converted;
    for (const auto &timing: thread_timings) {
        thread_timings_converted[timing.first] = sqsgenerator::python::helpers::vector_to_list(timing.second);
    }
    return py::make_tuple(wrapped_results, thread_timings_converted);
}


void set_log_level(boost::log::trivial::severity_level log_level = boost::log::trivial::info) {
    logging::core::get()->set_filter(logging::trivial::severity >= log_level);
}


BOOST_PYTHON_MODULE(iteration) {
        Py_Initialize();
        np::initialize();
            init_logging();

        py::enum_<iteration_mode>("IterationMode")
                .value("random", sqsgenerator::random)
                .value("systematic", sqsgenerator::systematic);

        py::enum_<boost::log::trivial::severity_level>("BoostLogLevel")
                .value("trace", boost::log::trivial::trace)
                .value("debug", boost::log::trivial::debug)
                .value("info", boost::log::trivial::info)
                .value("warning", boost::log::trivial::warning)
                .value("error", boost::log::trivial::error)
                .value("fatal", boost::log::trivial::fatal);

        py::class_<pair_shell_weights_t>("ShellWeights")
                .def(py::map_indexing_suite<pair_shell_weights_t>());
        //StructurePythonWrapper wrapper, double target_objective, np::ndarray parameter_weights, pair_shell_weights_t shell_weights, int iterations, int output_configurations, int iteration_mode, uint8_t prec
        py::class_<IterationSettingsPythonWrapper>("IterationSettings",py::init<StructurePythonWrapper, np::ndarray, np::ndarray, py::dict, int, int, py::list, iteration_mode>())
                .def(py::init<StructurePythonWrapper, np::ndarray, np::ndarray, py::dict, int, int, py::list, double, double, iteration_mode>())
                .def(py::init<StructurePythonWrapper, np::ndarray, np::ndarray, py::dict, int, int, py::list, py::list, double, double, iteration_mode>())
                .def_readonly("num_atoms", &IterationSettingsPythonWrapper::num_atoms)
                .def_readonly("num_shells", &IterationSettingsPythonWrapper::num_shells)
                .def_readonly("num_iterations", &IterationSettingsPythonWrapper::num_iterations)
                .def_readonly("num_species", &IterationSettingsPythonWrapper::num_species)
                .def_readonly("num_output_configurations", &IterationSettingsPythonWrapper::num_output_configurations)
                .def_readonly("mode", &IterationSettingsPythonWrapper::mode)
                .def_readonly("structure", &IterationSettingsPythonWrapper::structure)
                .def_readonly("target_objective", &IterationSettingsPythonWrapper::target_objective)
                .def_readonly("shell_weights", &IterationSettingsPythonWrapper::shell_weights)
                .def_readonly("parameter_weights", &IterationSettingsPythonWrapper::parameter_weights)
                .def_readonly("atol", &IterationSettingsPythonWrapper::atol)
                .def_readonly("rtol", &IterationSettingsPythonWrapper::rtol);

        py::def("set_log_level", &set_log_level);
        py::def("pair_sqs_iteration", &pair_sqs_iteration);

        py::list features;
#ifdef _OPENMP
        features.append("openmp");
#endif
#if defined(USE_MPI)
    features.append("mpi");
#endif
        py::scope().attr("__features__")= features;
        py::scope().attr("__version__") = py::make_tuple(VERSION_MAJOR, VERSION_MINOR, GIT_COMMIT_HASH, GIT_BRANCH);
}