from typing import Union

import numpy as np
import pytest

from sqsgenerator.core import StructureDouble, StructureFloat

try:
    from pymatgen.core import Structure

    HAVE_PYMATGEN = True
except ImportError:
    HAVE_PYMATGEN = False


def make_structure(
    structure_type: Union[type[StructureFloat], type[StructureDouble]],
) -> Union[StructureFloat, StructureDouble]:
    return structure_type(
        np.diag([1, 2, 3]),
        np.array([[0.0, 0.0, 0.0], [0.5, 0.5, 0.0], [0.0, 0.5, 0.5], [0.5, 0.0, 0.5]]),
        [4] * 2 + [13] * 2,
    )


@pytest.mark.parametrize("structure_type", [StructureFloat, StructureDouble])
def test_structure_equals(structure_type):
    assert make_structure(structure_type) == make_structure(structure_type)


@pytest.mark.parametrize("structure_type", [StructureFloat, StructureDouble])
def test_structure_supercell(structure_type):
    structure = make_structure(structure_type)
    with pytest.raises(TypeError):
        structure * 4
    with pytest.raises(TypeError):
        structure * "45"

    assert structure * (1, 1, 1) == structure

    assert structure * (2, 2, 2) == structure.supercell(2, 2, 2)

    supercell = structure * (1, 2, 3)
    assert len(supercell) == len(structure) * 6

    assert np.allclose(supercell.lattice, structure.lattice * np.diag([1, 2, 3]))


@pytest.mark.skipif(not HAVE_PYMATGEN, reason="pymatgen not installed")
@pytest.mark.parametrize("structure_type", [StructureFloat, StructureDouble])
def test_structure_supercell_pymatgen(structure_type):
    structure = make_structure(structure_type)

    def to_pymatgen(s: Union[StructureFloat, StructureDouble]) -> Structure:
        return Structure(
            s.lattice,
            s.symbols,
            s.frac_coords,
            coords_are_cartesian=False,
        )

    pymatgen_structure = to_pymatgen(structure)

    supercell = (1, 2, 3)
    assert pymatgen_structure * supercell == to_pymatgen(structure * supercell)
