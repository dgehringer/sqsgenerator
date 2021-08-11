import numbers
import itertools

import numpy as np
import typing as T
from sqsgenerator.core import Structure as Structure_, BadSettings
from sqsgenerator.core.fn import *
from sqsgenerator.compat import require, Feature, have_feature

from ase.atoms import Atoms


class Structure(Structure_):

    def __init__(self, lattice: np.ndarray, frac_coords: np.ndarray, symbols=T.List[str], pbc=(True, True, True)):
        super(Structure, self).__init__(lattice, frac_coords, symbols, pbc)
        self._symbols = np.array(tuple(map(attr('symbol'), self.species)))
        self._numbers = np.array(tuple(map(attr('Z'), self.species)))
        self._unique_species = set(np.unique(self._symbols))

    @property
    def symbols(self):
        return self._symbols

    @property
    def numbers(self):
        return self._numbers

    @property
    def num_unique_species(self):
        return len(self._unique_species)

    @property
    def unique_species(self):
        return self._unique_species

    def __len__(self):
        return self.num_atoms

    def __repr__(self):
        def group_symbols():
            for species, same in itertools.groupby(self._symbols):
                num_same = len(list(same))
                yield species if num_same == 1 else f'{species}{num_same}'

        formula = ''.join(group_symbols())
        return f'Structure({formula}, len={self.num_atoms})'

    def sorted(self):
        return self[np.argsort(self._numbers)]

    def __getitem__(self, item):
        if isinstance(item, numbers.Integral):
            if item < -self.num_atoms or item >= self.num_atoms:
                raise IndexError('Index out of range.')
            return self.species[item]

        if isinstance(item, (list, tuple, np.ndarray)):
            indices = np.array(item)
            if indices.dtype != int: raise TypeError('Only integer numbers cann be used for slicing')
            if indices.dtype == bool: indices = np.argwhere(indices).flatten()  # boolean mask slice
        elif isinstance(item, slice): indices = np.arange(self.num_atoms)[item]
        else: raise TypeError(f'Structure indices must not be of type {type(item)}')
        return Structure(self.lattice, self.frac_coords[indices], self.symbols[indices])

    def to_dict(self):
        return structure_to_dict(self)


unique_species = c_attr('species') | apply(attr('symbol')) | set
num_species = c_(unique_species) | len
symbols = c_attr('species') | apply(attr('symbol')) | list


@require(Feature.ase, Feature.pymatgen, condition=any)
def read_structure_from_file(settings: attrdict.AttrDict):
    known_readers = [ Feature.ase.value, Feature.pymatgen.value ]
    available_readers = list(filter(have_feature, known_readers))
    reader = settings.structure.get('reader', 'ase')
    if reader not in available_readers: raise BadSettings(f'Unnokwn reader specification "{reader}". Available readers are {available_readers}')
    reader_kwargs = settings.structure.get('args', {})
    reader_funcs = dict(ase=read_structure_file_with_ase, pymatgen=read_structure_file_with_pymatgen)
    return try_(reader_funcs[reader], settings.structure.file, **reader_kwargs, msg=f'read structure file "{settings.structure.file}"')


@require(Feature.pymatgen)
def to_pymatgen_structure(structure: Structure):
    from pymatgen.core import Structure as PymatgenStructure, Lattice as PymatgenLattice
    symbols = list(map(attr('symbol'), structure.species))
    lattice = PymatgenLattice(structure.lattice)
    return PymatgenStructure(lattice, symbols, structure.frac_coords, coords_are_cartesian=False)


@require(Feature.ase)
def to_ase_atoms(structure: Structure):
    from ase import Atoms
    numbers = list(map(attr('Z'), structure.species))
    return Atoms(cell=structure.lattice, scaled_positions=structure.frac_coords, numbers=numbers, pbc=structure.pbc)


@require(Feature.pymatgen)
def from_pymatgen_structure(structure) -> Structure:
    lattice = structure.lattice.matrix
    data = map(lambda site: (site.species_string, site.frac_coords), structure)
    species, frac_coords = transpose(data)
    return Structure(np.array(lattice), np.array(frac_coords), species, (True, True, True))


@require(Feature.ase)
def from_ase_atoms(atoms) -> Structure:
    lattice = np.array(atoms.cell)
    frac_coords = np.array(atoms.get_scaled_positions())
    if tuple(atoms.pbc) != (True, True, True): raise RuntimeWarning("At present I can only handle fully periodic structure. I'will overwrite ase.Atoms.pbc setting to (True, True, True)")
    return Structure(lattice, frac_coords, list(atoms.symbols), (True, True, True))


@require(Feature.ase)
def read_structure_file_with_ase(fn, **kwargs) -> Structure:
    import ase.io
    return from_ase_atoms(ase.io.read(fn, **kwargs))


@require(Feature.pymatgen)
def read_structure_file_with_pymatgen(fn, **kwargs) -> Structure:
    import pymatgen.core
    return from_pymatgen_structure(pymatgen.core.Structure.from_file(fn, **kwargs))


def make_supercell(structure: Structure, sa: int = 1, sb: int = 1, sc : int = 1):
    sizes = (sa, sb, sc)
    num_cells = np.prod(sizes)
    scale = np.reciprocal(np.array(sizes).astype(float))
    scaled_coords = np.vstack([scale]*structure.num_atoms) * structure.frac_coords
    num_atoms_supercell = num_cells * structure.num_atoms
    lattice_supercell = structure.lattice * np.diag(sizes)
    species_supercell = list(map(attr('symbol'), structure.species)) * num_cells

    frac_coords_supercell = []
    for ta, tb, tc in itertools.product(*map(range, sizes)):
        t = np.vstack([np.array([ta, tb, tc]*scale)]*structure.num_atoms)
        frac_coords_supercell.append(scaled_coords + t)
    frac_coords_supercell = np.vstack(frac_coords_supercell)

    assert frac_coords_supercell.shape == (num_atoms_supercell, 3)
    assert len(species_supercell) == num_atoms_supercell
    structure_supercell = Structure(lattice_supercell, frac_coords_supercell, species_supercell, (True, True, True))
    return structure_supercell


def structure_to_dict(structure: Structure):
    return dict(lattice=structure.lattice.tolist(), coords=structure.frac_coords.tolist(), species=symbols(structure))