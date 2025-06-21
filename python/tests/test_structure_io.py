import os.path
import tempfile

import numpy as np
import pytest

from sqsgenerator._adapters import (
    ase_formats,
    pymatgen_formats,
    read,
    sqsgen_formats,
    to_ase,
    to_pymatgen,
    write,
)
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

    assert (
        Structure.from_str(structure.dump(StructureFormat.cif), fmt="cif")
        == pymatgen_structure
    )


ALL_FORMATS = list(sqsgen_formats()) + [f"sqsgen.{fmt}" for fmt in sqsgen_formats()]

if HAVE_ASE:
    ALL_FORMATS += [f"ase.{fmt}" for fmt in ase_formats().keys()]
if HAVE_PYMATGEN:
    ALL_FORMATS += [f"pymatgen.{fmt}" for fmt in pymatgen_formats()]


@pytest.mark.parametrize("fmt", ALL_FORMATS)
@pytest.mark.parametrize("structure_type", [StructureFloat, StructureDouble])
def test_write_read_pymatgen(structure_type, fmt):
    structure = make_structure(structure_type) * (2, 2, 2)
    pymatgen_structure = to_pymatgen(structure)

    if fmt == "sqsgen.cif" or fmt == "cif":
        pytest.skip(f"Skipping {fmt} sqsgen CIF")

    with tempfile.TemporaryDirectory() as temp_dir:
        name = os.path.join(temp_dir, f"test.pymatgen.{fmt}")
        write(structure, name)
        loaded = to_pymatgen(read(name))
        assert pymatgen_structure == loaded
