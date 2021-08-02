import operator

import yaml
import click
import logging
import attrdict
import functools
import numpy as np
from sqsgenerator.compat import require, Feature
from sqsgenerator.core.data import Structure
import typing as T

class BadSettings(Exception):
    pass

def transpose(iterable: T.Iterable): return zip(*iterable)

def setup_logging():
    try:
        import rich.logging
    except ImportError: pass
    else:
        logging.basicConfig(
            level="NOTSET",
            format="%(message)s",
            datefmt="[%X]",
            handlers=[rich.logging.RichHandler(rich_tracebacks=True)]
        )


def get_function_logger(f: T.Callable):
    return logging.getLogger(f.__module__ + '.' + f.__name__)


def try_(f : T.Callable, *args, exc_type=Exception, raise_exc=True, return_success=False, msg=None, log_exc_info=False, **kwargs):
    try:
        result = f(*args, **kwargs)
        success = True
    except exc_type as e:
        if msg: get_function_logger(f).exception(f'Failed to {msg}', exc_info=e if log_exc_info else None)
        if raise_exc: raise
        result = None
        success = False
    return (success, result) if return_success else result

@require(Feature.pymatgen)
def to_pymatgen_structure(structure: Structure):
    from pymatgen.core import Structure as PymatgenStructure, Lattice as PymatgenLattice
    symbols = list(map(operator.attrgetter('symbol'), structure.species))
    lattice = PymatgenLattice(structure.lattice)
    return PymatgenStructure(lattice, symbols, structure.frac_coords, coords_are_cartesian=False)

@require(Feature.ase)
def to_ase_atoms(structure: Structure):
    from ase import Atoms
    numbers = list(map(operator.attrgetter('Z'), structure.species))
    return Atoms(cell=structure.lattice, scaled_positions=structure.frac_coords, numbers=numbers, pbc=structure.pbc)


@require(Feature.pymatgen)
def from_pymatgen_structure(structure):
    lattice = structure.lattice.matrix
    data = map(lambda site: (site.species_string, site.frac_coords), structure)
    species, frac_coords = transpose(data)
    return Structure(np.array(lattice), np.array(frac_coords), species, (True, True, True))

@require(Feature.ase)
def from_ase_atoms(atoms):
    lattice = np.array(atoms.cell)
    frac_coords = np.array(atoms.get_scaled_positions())
    if tuple(atoms.pbc) != (True, True, True): raise RuntimeWarning("At present I can only handle fully periodic structure. I'will overwrite ase.Atoms.pbc setting to (True, True, True)")
    return Structure(lattice, frac_coords, list(atoms.symbols), (True, True, True))


@require(Feature.ase)
def read_structure_file_ase(fn, **kwargs):
    import ase.io
    return from_ase_atoms(ase.io.read(fn, **kwargs))

@require(Feature.pymatgen)
def read_structure_file_pymatgen(fn, **kwargs):
    import pymatgen.core
    return from_pymatgen_structure(pymatgen.core.Structure.from_file(fn, **kwargs))

@require(Feature.ase, Feature.pymatgen, condition=any)
def read_structure_from_file(settings: attrdict.AttrDict):
    known_readers = [ Feature.ase.value, Feature.pymatgen.value ]
    available_readers = list(filter(compat.have_feature, known_readers))
    reader = settings.structure.get('reader', 'ase')
    if reader not in available_readers: raise BadSettings(f'Unnokwn reader specification "{reader}". Available readers are {available_readers}')
    reader_kwargs = settings.structure.get('args', {})
    reader_funcs = dict(ase=read_structure_file_ase, pymatgen=read_structure_file_pymatgen)
    return try_(reader_funcs[reader], settings.structure.file, **reader_kwargs, msg=f'read structure file "{settings.structure.file}"')



def read_structure(settings : attrdict.AttrDict):
    needed_fields = {'lattice', 'coords', 'species'}
    s = settings.structure
    if isinstance(s, Structure): structure = s
    elif compat.have_feature(Feature.ase):
        from ase import Atoms
        if isinstance(s, Atoms): structure = from_ase_atoms(s)
    elif compat.have_feature(Feature.pymatgen):
        from pymatgen.core import Structure as PymatgenStructure
        if isinstance(s, PymatgenStructure): structure = s
    elif 'file' in settings.structure:
        read_structure_from_file(settings)
    elif all(field in settings.structure for field in needed_fields):
        lattice = np.array(settings.structure.lattice)
        coords = np.array(settings.structure.coords)
        species = list(settings.structure.species)
        structure = Structure(lattice, coords, species, (True, True, True))

    return structure



if __name__ == '__main__':

    setup_logging()
    import os
    import compat
    print(os.getcwd())
    d = attrdict.AttrDict(yaml.safe_load(open('examples/cs-cl.sqs.yaml')))
    print(compat.have_ase(), compat.have_pymatgen(), compat.have_pyiron())

    read_structure(d)