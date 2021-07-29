//
// Created by dominik on 14.07.21.
//
#include "data.hpp"
#include "helpers.hpp"

using namespace sqsgenerator::python;
using namespace sqsgenerator;

SQSResultPythonWrapper::SQSResultPythonWrapper(const SQSResult &other) : m_handle(new SQSResult(other)) {}
SQSResultPythonWrapper::SQSResultPythonWrapper(SQSResult &&other) : m_handle(new SQSResult(std::forward<SQSResult>(other))) {}

double SQSResultPythonWrapper::objective(){
    return m_handle->objective();
}

cpp_int SQSResultPythonWrapper::rank() {
    return m_handle->rank();
}

np::ndarray SQSResultPythonWrapper::configuration() {
    return helpers::to_flat_numpy_array<species_t>(
            m_handle->configuration().data(),
            m_handle->configuration().size()
    );
}

np::ndarray SQSResultPythonWrapper::parameters(py::tuple const &shape) {
    return helpers::to_flat_numpy_array<double>(
            m_handle->storage().data(),
            m_handle->storage().size()).reshape(shape);
}


std::shared_ptr<SQSResult> SQSResultPythonWrapper::handle() {
    return m_handle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


StructurePythonWrapper::StructurePythonWrapper(const_array_2d_ref_t lattice, const_array_2d_ref_t frac_coords, configuration_t species, std::array<bool, 3> pbc) :
    m_handle(std::shared_ptr<atomistics::Structure>(
                    new atomistics::Structure(lattice, frac_coords, species,pbc)))
                    {

}

StructurePythonWrapper::StructurePythonWrapper(np::ndarray lattice, np::ndarray frac_coords, py::object symbols, py::tuple pbc):
        StructurePythonWrapper(
            helpers::ndarray_to_multi_array<const_array_2d_ref_t>(lattice),
            helpers::ndarray_to_multi_array<const_array_2d_ref_t>(frac_coords),
            sqsgenerator::utils::atomistics::Atoms::symbol_to_z(helpers::list_to_vector<std::string>(symbols)),
        {py::extract<bool>(pbc[0]),py::extract<bool>(pbc[1]), py::extract<bool>(pbc[2])})
        {}

np::ndarray StructurePythonWrapper::lattice() {
    return helpers::multi_array_to_ndarray(m_handle->lattice());
}

np::ndarray StructurePythonWrapper::frac_coords() {

    return helpers::multi_array_to_ndarray(m_handle->frac_coords());
}

py::list StructurePythonWrapper::species() {
    return helpers::vector_to_list(m_handle->species());
}

py::tuple StructurePythonWrapper::pbc() {
    auto pbc {m_handle->pbc()};
    return py::make_tuple(pbc[0], pbc[1], pbc[2]);
}

np::ndarray StructurePythonWrapper::distance_vecs() {
    return helpers::multi_array_to_ndarray(m_handle->distance_vecs());
}

np::ndarray StructurePythonWrapper::distance_matrix() {
    return helpers::multi_array_to_ndarray(m_handle->distance_matrix());
}

/*
np::ndarray StructurePythonWrapper::shell_matrix(double atol, double rtol) {
    return helpers::multi_array_to_ndarray(m_handle->shell_matrix(atol, rtol));
}

np::ndarray StructurePythonWrapper::shell_matrix(const py::list &distances, double atol, double rtol) {
    return helpers::multi_array_to_ndarray(m_handle->shell_matrix(helpers::list_to_vector<double>(distances), atol, rtol));
}
*/

size_t StructurePythonWrapper::num_atoms() {
    return m_handle->num_atoms();
}

std::shared_ptr<atomistics::Structure> StructurePythonWrapper::handle(){
    return m_handle;
}