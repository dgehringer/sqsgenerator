
"""
Utility functions to convert ``sqsgenerator.core.Structure`` to ``ase.Atoms`` and ``pymatgen.core.Structure``
and vice versa
"""


import numpy as np
import typing as T
from operator import attrgetter as attr
from sqsgenerator.core import Structure
from sqsgenerator.compat import Feature as F, require


def transpose(iterable: T.Iterable):
    return zip(*iterable)


@require(F.pymatgen)
def to_pymatgen_structure(structure: Structure):
    """
    Convert structure {structure} to a ``pymatgen.core.Structure`` object

    :param structure: the structure to convert
    :type structure: Structure
    :return: the ``pymatgen.core.Structure`` object
    :rtype: pymatgen.core.Structure
    """
    from pymatgen.core import Structure as PymatgenStructure, Lattice as PymatgenLattice
    structure = structure.without_vacancies()
    symbols = list(map(attr('symbol'), structure.species))
    lattice = PymatgenLattice(structure.lattice)
    return PymatgenStructure(lattice, symbols, structure.frac_coords, coords_are_cartesian=False, validate_proximity=True)


@require(F.ase)
def to_ase_atoms(structure: Structure):
    """
    Convert structure {structure} to a ``ase.Atoms`` object

    :param structure: the structure to convert
    :type structure: Structure
    :return: the ``ase.Atoms`` object
    :rtype: ase.Atoms
    """
    from ase import Atoms
    structure = structure.without_vacancies()
    numbers = list(map(attr('Z'), structure.species))
    return Atoms(cell=structure.lattice, scaled_positions=structure.frac_coords, numbers=numbers, pbc=structure.pbc)


@require(F.pymatgen)
def from_pymatgen_structure(structure) -> Structure:
    """
    Convert the ``pymatgen.core.Structure`` {structure} object to a ``sqsgenerator.core.Structure`` object

    :param structure: the structure to convert
    :type structure: pymatgen.core.Structure
    :return: the structure object
    :rtype: Structure
    """
    lattice = structure.lattice.matrix
    data = map(lambda site: (site.species_string, site.frac_coords), structure)
    species, frac_coords = transpose(data)
    return Structure(np.array(lattice), np.array(frac_coords), species, (True, True, True))


@require(F.ase)
def from_ase_atoms(atoms) -> Structure:
    """
    Convert the ``ase.Atoms`` {structure} object to a ``sqsgenerator.core.Structure`` object

    :param atoms: the structure to convert
    :type atoms: ase.Atoms
    :return: the structure object
    :rtype: Structure
    """
    lattice = np.array(atoms.cell)
    frac_coords = np.array(atoms.get_scaled_positions()).copy()
    if tuple(atoms.pbc) != (True, True, True):
        raise RuntimeWarning("At present I can only handle fully periodic structures. "
                             "I'will overwrite ase.Atoms.pbc setting to (True, True, True)")
    return Structure(lattice, frac_coords, list(atoms.symbols), (True, True, True))

