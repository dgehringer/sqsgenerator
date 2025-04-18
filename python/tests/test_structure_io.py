import numpy as np
import pytest

from sqsgenerator.core import (
    StructureDouble,
    StructureFloat,
    StructureFormat,
)

try:
    from pymatgen.core import Structure
    from pymatgen.io.vasp import Poscar
except ImportError:
    HAVE_PYMATGEN = False
else:
    HAVE_PYMATGEN = True

try:
    from ase import Atoms
except ImportError:
    HAVE_ASE = False
else:
    HAVE_ASE = True


def make_structure(
    structure_type: type[StructureFloat] | type[StructureDouble],
) -> StructureFloat | StructureDouble:
    return structure_type(
        np.diag([1, 2, 3]),
        np.array([[0.0, 0.0, 0.0], [0.5, 0.5, 0.0], [0.0, 0.5, 0.5], [0.5, 0.0, 0.5]]),
        [4] * 4,
    )


@pytest.mark.parametrize("structure_type", [StructureFloat, StructureDouble])
@pytest.mark.parametrize(
    "fmt", [StructureFormat.json_pymatgen, StructureFormat.json_sqsgen]
)
def test_structure_json(structure_type, fmt):
    structure = make_structure(structure_type)
    assert structure == structure_type.from_json(structure.dump(fmt), fmt)


@pytest.mark.parametrize("structure_type", [StructureFloat, StructureDouble])
def test_structure_poscar(structure_type):
    structure = make_structure(structure_type)
    assert structure == structure_type.from_poscar(
        structure.dump(StructureFormat.poscar)
    )


@pytest.mark.parametrize("structure_type", [StructureFloat, StructureDouble])
def test_structure_binary(structure_type):
    structure = make_structure(structure_type)
    assert structure == structure_type.from_bytes(structure.bytes())


@pytest.mark.parametrize("structure_type", [StructureFloat, StructureDouble])
def test_structure_pymatgen(structure_type):
    structure = make_structure(structure_type) * (2, 2, 2)

    def to_pymatgen(s: StructureFloat | StructureDouble) -> Structure:
        return Structure(
            s.lattice,
            s.symbols,
            s.frac_coords,
            coords_are_cartesian=False,
        )

    pymatgen_structure = to_pymatgen(structure)

    assert (
        Poscar.from_str(structure.dump(StructureFormat.poscar)).structure
        == pymatgen_structure
    )
    assert (
        Structure.from_str(structure.dump(StructureFormat.json_pymatgen), fmt="json")
        == pymatgen_structure
    )
