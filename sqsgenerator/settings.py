import collections
import enum
import operator

import yaml
import click
import logging
import attrdict
import functools
import itertools
import numpy as np
import typing as T
from sqsgenerator.compat import require, Feature
from sqsgenerator.core.data import Structure
from sqsgenerator.core.iteration import IterationMode

__parameter_registry = collections.OrderedDict({})


class BadSettings(Exception):
    pass

attr = operator.attrgetter
item = operator.itemgetter

def random_mode(settings): return settings.mode == IterationMode.random

def if_(condition):
    def then_(th_val):
        def else_(el_val):
            def stmt_(settings): return th_val if condition(settings) else el_val
            return stmt_
        return else_
    return then_

def get_function_logger(f: T.Callable) -> logging.Logger:
    return logging.getLogger(f.__module__ + '.' + f.__name__)


class Default(enum.Enum):
    NoDefault = 0


def parameter(name: str, default: T.Optional[T.Any] = Default.NoDefault, required: T.Union[T.Callable, bool]=False, key: T.Union[T.Callable, str] = None):
    if key is None: key = name
    if isinstance(required, bool): get_required = lambda *_: required
    if isinstance(key, str): get_key = lambda *_: key
    if not callable(default): get_default = lambda *_: default
    have_default = default != Default.NoDefault
    # if not required and default is None: raise RuntimeWarning(f'Option "{name}" is optional but no default value was specfied. "None" will be used')

    def _decorator(f: T.Callable):
        @functools.wraps(f)
        def _wrapped(settings: attrdict.AttrDict):
            is_required = get_required(settings)
            k = get_key(settings)
            if k not in settings:
                nonlocal name
                if is_required:
                    if not have_default:
                        raise BadSettings(f'Required parameter "{name}" was not found')
                    else:
                        df = get_default(settings)
                        get_function_logger(f).warning(f'Parameter "{name}" was not found defaulting to: "{df}"')
                        settings[k] = df
                        return df
            else: return f(settings)

        global __parameter_registry
        __parameter_registry[name] = _wrapped
        return _wrapped

    return _decorator


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


def try_(f: T.Callable, *args, exc_type: T.Type = Exception, raise_exc: bool=True, return_success: bool =False, msg: T.Optional[str]=None, log_exc_info: bool=False, **kwargs) -> T.Any:
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


@parameter('structure', required=True)
def read_structure(settings : attrdict.AttrDict) -> Structure:
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
    else: raise BadSettings('Cannot read structure from the settings')

    if 'supercell' in settings.structure:
        sizes = settings.structure.supercell
        if len(sizes) != 3: raise BadSettings('To create a supercell you need to specify three lengths')
        structure = make_supercell(structure, *sizes)

    return structure


@parameter('mode', default=IterationMode.random, required=True)
def read_mode(settings: attrdict.AttrDict):
    if settings.mode not in IterationMode.names:
        raise BadSettings(f'Unknown iteration mode "{settings.mode}". Available iteration modes are {list(IterationMode.names.keys())}')
    return IterationMode.names[settings.mode]

@parameter('iterations', default=if_(random_mode)(1e5)(-1), required=if_(random_mode)(True)(False))
def read_iterations(settings: attrdict.AttrDict):
    print(settings.iterations)

def process_settings(settings: attrdict.AttrDict):
    print(__parameter_registry)
    for param, processor in __parameter_registry.items():
        processor(settings)
    print(settings)

if __name__ == '__main__':

    setup_logging()
    import os
    import compat
    print(os.getcwd())
    d = attrdict.AttrDict(yaml.safe_load(open('examples/cs-cl.sqs.yaml')))
    print(compat.have_ase(), compat.have_pymatgen(), compat.have_pyiron(), compat.have_mpi4py())

    process_settings(d)