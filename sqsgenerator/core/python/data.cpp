//
// Created by dominik on 14.07.21.
//
#include "data.hpp"

using namespace sqsgenerator::python;
using namespace sqsgenerator;

SQSResultPythonWrapper::SQSResultPythonWrapper(const SQSResult &other) : m_handle(other) {}

double SQSResultPythonWrapper::objective(){
    return m_handle.objective();
}

cpp_int SQSResultPythonWrapper::rank() {
    return m_handle.rank();
}

np::ndarray SQSResultPythonWrapper::configuration() {
    return helpers::to_flat_numpy_array<species_t>(
            m_handle.configuration().data(),
            m_handle.configuration().size()
    );
}

np::ndarray SQSResultPythonWrapper::parameters(py::tuple const &shape) {
    return helpers::to_flat_numpy_array<double>(
            m_handle.storage().data(),
            m_handle.storage().size()).reshape(shape);
}


const SQSResult & SQSResultPythonWrapper::handle() {
    return m_handle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StructurePythonWrapper::StructurePythonWrapper(np::ndarray lattice, np::ndarray frac_coords, py::object symbols, py::tuple pbc):
    m_handle(
            helpers::ndarray_to_multi_array<double, 2>(lattice),
            helpers::ndarray_to_multi_array<double,2>(frac_coords),
            helpers::list_to_vector<std::string>(symbols),
    {py::extract<bool>(pbc[0]),py::extract<bool>(pbc[1]), py::extract<bool>(pbc[2])}) { }


np::ndarray StructurePythonWrapper::lattice() {
    return helpers::multi_array_to_ndarray<const_array_2d_ref_t,2>(m_handle.lattice());
}

np::ndarray StructurePythonWrapper::frac_coords() {
    return helpers::multi_array_to_ndarray<const_array_2d_ref_t,2>(m_handle.frac_coords());
}

py::list StructurePythonWrapper::species() {
    return helpers::vector_to_list(m_handle.species());
}

py::tuple StructurePythonWrapper::pbc() {
    auto pbc {m_handle.pbc()};
    return py::make_tuple(pbc[0], pbc[1], pbc[2]);
}

np::ndarray StructurePythonWrapper::distance_vecs() {
    return helpers::multi_array_to_ndarray<const_array_3d_ref_t , 3>(m_handle.distance_vecs());
}

np::ndarray StructurePythonWrapper::distance_matrix() {
    return helpers::multi_array_to_ndarray<const_array_2d_ref_t , 2>(m_handle.distance_matrix());
}

np::ndarray StructurePythonWrapper::shell_matrix(uint8_t prec) {
    return helpers::multi_array_to_ndarray<const_pair_shell_matrix_ref_t, 2>(m_handle.shell_matrix(prec));
}

size_t StructurePythonWrapper::num_atoms() {
    return m_handle.num_atoms();
}

atomistics::Structure& StructurePythonWrapper::handle() {
    return m_handle;
}