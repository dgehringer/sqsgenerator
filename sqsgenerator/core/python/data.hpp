//
// Created by dominik on 14.07.21.
//

#ifndef SQSGENERATOR_DATA_HPP
#define SQSGENERATOR_DATA_HPP
#include "types.hpp"
#include "result.hpp"
#include "atomistics.hpp"
#include <boost/python.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/python/numpy.hpp>

namespace py = boost::python;
namespace np = boost::python::numpy;
namespace atomistics = sqsgenerator::utils::atomistics;

namespace sqsgenerator::python {

    class SQSResultPythonWrapper {
    private:
        std::shared_ptr<SQSResult> m_handle;
    public:

        explicit SQSResultPythonWrapper(const SQSResult &other);
        SQSResultPythonWrapper(SQSResult &&other);
        double objective();
        rank_t rank();
        np::ndarray configuration();
        np::ndarray parameters(py::tuple const &shape);
        std::shared_ptr<SQSResult> handle();
    };

    class StructurePythonWrapper {
    private:
        std::shared_ptr<atomistics::Structure> m_handle;
    public:
        StructurePythonWrapper(np::ndarray lattice, np::ndarray frac_coords, py::object symbols);
        StructurePythonWrapper(np::ndarray lattice, np::ndarray frac_coords, py::object symbols, py::object pbc);
        StructurePythonWrapper(const_array_2d_ref_t lattice, const_array_2d_ref_t frac_coords, configuration_t species, std::array<bool, 3> pbc);
        StructurePythonWrapper(const StructurePythonWrapper &other) = default;
        np::ndarray lattice();
        np::ndarray frac_coords();
        py::list species();
        py::tuple pbc();
        np::ndarray distance_vecs();
        np::ndarray distance_matrix();
        size_t num_atoms();
        std::shared_ptr<atomistics::Structure> handle();
        StructurePythonWrapper sorted();
        StructurePythonWrapper rearranged(py::object order);
    };

    typedef std::vector<SQSResultPythonWrapper> SQSResultCollection;
}

#endif //SQSGENERATOR_DATA_HPP
