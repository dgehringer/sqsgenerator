import numpy as np
import pytest
from typing import Type, Union
from sqsgenerator.core import (
    StructureFloat,
    StructureDouble,
    StructureFormat,
    __version__,
)

try:
    from pymatgen.core import Structure
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
    structure_type: Union[Type[StructureFloat], Type[StructureDouble]],
) -> Union[StructureFloat, StructureDouble]:
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
