import sys

import numpy as np
import pytest
from typing import Type, Union
from sqsgenerator.core import StructureFloat, StructureDouble, __version__


def make_structure(
    structure_type: Union[Type[StructureFloat], Type[StructureDouble]],
) -> Union[StructureFloat, StructureDouble]:
    return structure_type(
        np.diag([1, 2, 3]),
        np.array([[0.0, 0.0, 0.0], [0.5, 0.5, 0.0], [0.0, 0.5, 0.5], [0.5, 0.0, 0.5]]),
        [4] * 4,
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
