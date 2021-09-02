import numbers
import itertools
import numpy as np
import typing as T
from operator import attrgetter as attr
from sqsgenerator.core.core import Structure as Structure_, Atom


class Structure(Structure_):

    def __init__(self, lattice: np.ndarray, frac_coords: np.ndarray, symbols=T.List[str], pbc=(True, True, True)):
        super(Structure, self).__init__(lattice, frac_coords, symbols, pbc)
        self._symbols = np.array(list(map(attr('symbol'), self.species)))
        self._numbers = np.fromiter(map(attr('Z'), self.species), dtype=int, count=self.num_atoms)
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
            if indices.dtype == bool:
                indices = np.argwhere(indices).flatten()
            elif indices.dtype != int:
                raise TypeError('Only integer numbers cann be used for slicing')
             # boolean mask slice
        elif isinstance(item, slice):
            indices = np.arange(self.num_atoms)[item]
        else:
            raise TypeError(f'Structure indices must not be of type {type(item)}')
        return Structure(self.lattice, self.frac_coords[indices], self.symbols[indices])

    def to_dict(self):
        return structure_to_dict(self)

    def slice_with_species(self, species, which=None):
        which = which or tuple(range)
        return self.with_species(species, which=which)[which]

    def with_species(self, species, which=None):
        which = which or tuple(range)
        species = list(species)
        if len(species) < 1:
            raise ValueError('Cannot create an empty structure')
        if len(which) != len(species):
            raise ValueError('Number of species does not match the number of specified atoms')
        new_symbols = self.symbols.copy()
        new_symbols[np.array(which)] = species
        return Structure(self.lattice, self.frac_coords, new_symbols.tolist())


def structure_to_dict(structure: Structure):
    return dict(lattice=structure.lattice.tolist(), coords=structure.frac_coords.tolist(), species=structure.symbols.tolist())


def make_supercell(structure: Structure, sa: int = 1, sb: int = 1, sc : int = 1):
    sizes = (sa, sb, sc)
    num_cells = np.prod(sizes)
    num_atoms_supercell = structure.num_atoms * num_cells
    scale = np.diag(sizes).astype(float)
    iscale = np.linalg.inv(scale)
    supercell_lattice = scale @ structure.lattice

    scaled_fc = structure.frac_coords @ iscale.T

    def make_translation_vector(a, b, c):
        return np.tile(np.array([a, b, c]) @ iscale, [structure.num_atoms, 1])

    supercell_coords = np.vstack([
        scaled_fc + make_translation_vector(*shift)
        for shift in itertools.product(*map(range, sizes))
    ])

    supercell_species = np.tile(structure.symbols, num_cells).tolist()

    assert supercell_coords.shape == (num_atoms_supercell, 3)
    assert len(supercell_species) == num_atoms_supercell
    structure_supercell = Structure(supercell_lattice, supercell_coords, supercell_species, (True, True, True))
    return structure_supercell