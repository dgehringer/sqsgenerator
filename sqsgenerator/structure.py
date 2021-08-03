import attrdict
import itertools
import numpy as np
from sqsgenerator.fn import attr, item
from sqsgenerator.compat import require, Feature
from sqsgenerator.core.data import Structure


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


@require(Feature.ase, Feature.pymatgen, condition=any)
def read_structure_from_file(settings: attrdict.AttrDict):
    known_readers = [ Feature.ase.value, Feature.pymatgen.value ]
    available_readers = list(filter(compat.have_feature, known_readers))
    reader = settings.structure.get('reader', 'ase')
    if reader not in available_readers: raise BadSettings(f'Unnokwn reader specification "{reader}". Available readers are {available_readers}')
    reader_kwargs = settings.structure.get('args', {})
    reader_funcs = dict(ase=read_structure_file_with_ase, pymatgen=read_structure_file_with_pymatgen)
    return try_(reader_funcs[reader], settings.structure.file, **reader_kwargs, msg=f'read structure file "{settings.structure.file}"')


def make_supercell(structure: Structure, sa: int = 1, sb: int = 1, sc : int = 1):
    sizes = (sa, sb, sc)
    num_cells = np.prod(sizes)
    scale = np.reciprocal(np.array(sizes).astype(float))

    num_atoms_supercell = num_cells * structure.num_atoms
    lattice_supercell = structure.lattice * np.diag(sizes)
    species_supercell = list(map(attr('symbol'), structure.species)) * num_cells

    frac_coords_supercell = []
    for ta, tb, tc in itertools.product(*map(range, sizes)):
        t = np.vstack([np.array([ta, tb, tc]*scale)]*structure.num_atoms)
        frac_coords_supercell.append(structure.frac_coords + t)
    frac_coords_supercell = np.vstack(frac_coords_supercell)

    assert frac_coords_supercell.shape == (num_atoms_supercell, 3)
    assert len(species_supercell) == num_atoms_supercell
    structure_supercell = Structure(lattice_supercell, frac_coords_supercell, species_supercell, (True, True, True))
    return structure_supercell