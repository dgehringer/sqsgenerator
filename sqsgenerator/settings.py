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
from sqsgenerator.fn import *
from sqsgenerator.structure import Structure, read_structure_from_file, make_supercell, from_ase_atoms, from_pymatgen_structure
from sqsgenerator.compat import require, Feature, have_mpi_support
from sqsgenerator.core.iteration import IterationMode
from sqsgenerator.core.utils import default_shell_distances


__parameter_registry = collections.OrderedDict({})

ATOL = 1e-5
RTOL = 1e-8


def parameter(name: str, default: T.Optional[T.Any] = Default.NoDefault, required: T.Union[T.Callable, bool]=True, key: T.Union[T.Callable, str] = None):
    if key is None: key = name
    get_required = lambda *_: required if isinstance(required, bool) else required
    get_key = lambda *_: key if isinstance(key, str) else key
    get_default = (lambda *_: default) if not callable(default) else default

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
                        return df
            else: return f(settings)

        global __parameter_registry
        __parameter_registry[name] = _wrapped
        return _wrapped

    return _decorator


class BadSettings(Exception):
    pass


def random_mode(settings): return settings.mode == IterationMode.random


def unique_species(settings): return set(sp.symbol for sp in settings.structure.species)


def num_species(settings): return len(unique_species(settings))


def num_shells(settings): return len(settings.shell_weights)


def num_sites_on_sublattice(settings, sublattice): return sum(sp.symbol.lower() == sublattice.lower() for sp  in settings.structure.species) if sublattice != 'all' else settings.structure.num_atoms



def get_function_logger(f: T.Callable) -> logging.Logger:
    return logging.getLogger(f.__module__ + '.' + f.__name__)


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


@parameter('atol', default=ATOL)
def read_atol(settings : attrdict.AttrDict):
    return settings.atol


@parameter('rtol', default=RTOL)
def read_atol(settings : attrdict.AttrDict):
    return settings.rtol


@parameter('structure')
def read_structure(settings : attrdict.AttrDict) -> Structure:
    needed_fields = {'lattice', 'coords', 'species'}
    s = settings.structure
    if isinstance(s, Structure): structure = s
    elif compat.have_feature(Feature.ase):
        from ase import Atoms
        if isinstance(s, Atoms): structure = from_ase_atoms(s)
    elif compat.have_feature(Feature.pymatgen):
        from pymatgen.core import Structure as PymatgenStructure
        if isinstance(s, PymatgenStructure): structure = from_pymatgen_structure(s)
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
    return int(float(settings.iterations))


@parameter('shell_distances', default=lambda s: default_shell_distances(s.structure, s.atol, s.rtol), required=True)
def read_shell_distances(settings: attrdict.AttrDict):
    return settings.shell_distances


@parameter('shell_weights', default=lambda settings: {i: 1.0/i for i, distance in enumerate(settings.shell_distances[1:], start=1)}, required=True)
def read_shell_weights(settings: attrdict.AttrDict):
    return settings.shell_weights


@parameter('pair_weights', default=lambda s: np.ones((num_species(s), num_species(s))), required=True)
def read_pair_weights(settings: attrdict.AttrDict):
    nums = num_species(settings)
    if isinstance(settings.pair_weights, (list, tuple)):
        w = np.array(settings.pair_weights).astype(float)
        if w.shape == (nums, nums): return w
        else: raise BadSettings(f'The "pair_weights" you have specified has a wrong shape ({w.shape}). Expected {(nums, nums)}')
    else:
        raise BadSettings(f'As "pair_weights" I do expect a {nums}x{nums} matrix, since your structure contains {nums} different species')


@parameter('target_objective', default=lambda s: np.zeros((num_shells(s), num_species(s), num_species(s))), required=True)
def read_target_objective(settings: attrdict.AttrDict):
    nums = num_species(settings)
    nshells = len(settings.shell_weights)
    if isinstance(settings.target_objective, (int, float)):
        o = np.ones((nshells, nums, nums)).astype(float)
        for index, (shell_index, shell_weight) in enumerate(sorted(settings.shell_weights.items(), key=item(0))):
            o[index,:,:] = settings.target_objective * shell_weight
        return o
    elif isinstance(settings.target_objective, (list, tuple)):
        o = np.array(settings.target_objective).astype(float)
        if o.ndim == 3:
            expected_shape = (nshells, nums, nums)
            if o.shape != expected_shape: raise BadSettings(f'The 3D "pair_weights" you have specified has a wrong shape ({o.shape}). Expected {expected_shape}')
            else: return o
        elif o.ndim == 2:
            expected_shape = (nums, nums)
            if o.shape != expected_shape: raise BadSettings(f'The 2D "pair_weights" you have specified has a wrong shape ({o.shape}). Expected {expected_shape}')
            else:
                f = np.ones((nshells, nums, nums)).astype(float)
                for index, (shell_index, shell_weight) in enumerate(sorted(settings.shell_weights.items(), key=item(0))):
                    f[index, :, :] = o * shell_weight
                return f
        else: raise BadSettings(f'The "pair_weights" you have specified has a {o.ndim} dimensions. I only understand 2 and 3')
    else:
        raise BadSettings(f'Cannot interpret "target_objective" setting. Acceptable values are a single number, {nums}x{nums} or {nshells}x{nums}x{nums} matrices!')


@parameter('threads_per_rank', default=(-1,), required=True)
def read_threads_per_rank(settings: attrdict.AttrDict):
    if isinstance(settings.threads_per_rank, (float, int)):
        return [int(settings.threads_per_rank)]
    elif isinstance(settings.threads_per_rank, (list, tuple)):
        if len(settings.threads_per_rank) != 1:
            if not have_mpi_support(): raise BadSettings(f'The module sqsgenerator.core.iteration was not compiled with MPI support')
        return list(settings.threads_per_rank)
    else: raise BadSettings(f'Cannot interpret "threads_per_rank" setting.')


@parameter('composition', required=True)
def read_composition(settings: attrdict.AttrDict):
    if not isinstance(settings.composition, dict): raise BadSettings(f'Cannot interpret "composition" setting. I expected a dictionary')
    allowed_sublattices = {'all',}.union(unique_species(settings))
    for sublattice in settings.composition.keys():
        if sublattice not in allowed_sublattices: raise BadSettings(f'The structure does not have an "{sublattice}" subllatice. Possible values would be {allowed_sublattices}')
        print(num_sites_on_sublattice(settings, sublattice))



def process_settings(settings: attrdict.AttrDict):
    for param, processor in __parameter_registry.items():
        settings[param] = processor(settings)
    return settings

if __name__ == '__main__':

    setup_logging()
    import os
    import compat
    # print(os.getcwd())
    d = attrdict.AttrDict(yaml.safe_load(open('examples/cs-cl.sqs.yaml')))
    # print(compat.have_ase(), compat.have_pymatgen(), compat.have_pyiron(), compat.have_mpi4py())
    import sqsgenerator.core.iteration
    print(sqsgenerator.core.iteration.__version__)
    process_settings(d)
    print(compat.have_feature(Feature.rich))