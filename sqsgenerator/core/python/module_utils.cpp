//
// Created by dominik on 29.07.21.
//

//
// Created by dominik on 30.05.21.
//

#include "data.hpp"
#include "rank.hpp"
#include "utils.hpp"
#include "version.hpp"
#include "helpers.hpp"

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

rank_t rank_configuration_internal(const configuration_t &conf) {
    auto nspecies {sqsgenerator::utils::unique_species(conf).size()};
    auto [_, packed_configuration] = sqsgenerator::utils::pack_configuration(conf);
    return sqsgenerator::utils::rank_permutation(packed_configuration, nspecies);
}

rank_t rank_structure(StructurePythonWrapper &s) {
    return rank_configuration_internal(s.handle()->configuration());
}

rank_t rank_configuration(const py::object &iterable) {
    return rank_configuration_internal(helpers::list_to_vector<species_t>(iterable));
}

rank_t total_permutations_internal(const configuration_t &conf) {
    auto [_, packed_configuration] = sqsgenerator::utils::pack_configuration(conf);
    return sqsgenerator::utils::total_permutations(packed_configuration);
}

rank_t total_permutations_iterable(const py::object &iterable) {
    return total_permutations_internal(helpers::list_to_vector<species_t>(iterable));
}

rank_t total_permutations_structure(StructurePythonWrapper &s) {
    return total_permutations_internal(s.handle()->configuration());
}

configuration_t make_rank_internal(const configuration_t &conf, rank_t &rank) {
    auto total_perms = total_permutations_internal(conf);
    auto [_, packed_configuration] = sqsgenerator::utils::pack_configuration(conf);
    auto hist = utils::configuration_histogram(packed_configuration);
    configuration_t result(conf);
    utils::unrank_permutation(result, hist, total_perms, rank);
    return result;
}

py::list make_rank_iterable(const py::object &iterable, rank_t &rank) {
    return helpers::vector_to_list(make_rank_internal(helpers::list_to_vector<species_t>(iterable), rank));
}

StructurePythonWrapper make_rank_structure(StructurePythonWrapper &s, rank_t &rank) {
    return StructurePythonWrapper()
}

BOOST_PYTHON_MODULE(utils) {
    Py_Initialize();
    py::def("rank_structure", &rank_structure);
    py::def("rank_configuration", &rank_configuration);

    py::def("total_permutations", &total_permutations_iterable);
    py::def("total_permutations", &total_permutations_structure);

    py::scope().attr("__version__") = py::make_tuple(VERSION_MAJOR, VERSION_MINOR, GIT_COMMIT_HASH, GIT_BRANCH);
}
