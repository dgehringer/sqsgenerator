//
// Created by dominik on 29.09.21.
//

#include "helpers.hpp"

namespace sqsgenerator::python::helpers {

    composition_t convert_composition(const py::dict &composition) {
        py::list keys = composition.keys();
        composition_t result;
        for (auto i = 0; i < py::len(keys); i++) {
            auto distributed_species{py::extract<species_t>(keys[i])};
            py::dict sublattice_spec = py::extract<py::dict>(composition[keys[i]]);
            py::list sublattice_spec_keys = sublattice_spec.keys();
            std::map<species_t, int> sublattice_spec_internal;

            for (auto j = 0; j < py::len(sublattice_spec_keys); j++) {
                auto destination_lattice{py::extract<species_t>(sublattice_spec_keys[j])};
                auto distributed_num_atoms{py::extract<int>(sublattice_spec[sublattice_spec_keys[j]])};
                sublattice_spec_internal.emplace(destination_lattice, distributed_num_atoms);
            }

            result.emplace(distributed_species, sublattice_spec_internal);
        }
        return result;
    }
}

