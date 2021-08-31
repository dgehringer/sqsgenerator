import collections
import collections.abc
import numbers
import typing as T
from functools import partial
from itertools import repeat, chain
from operator import attrgetter as attr, itemgetter as item

import numpy as np
from attrdict import AttrDict

from sqsgenerator.adapters import from_ase_atoms, from_pymatgen_structure
from sqsgenerator.compat import Feature, have_mpi_support, have_feature
from sqsgenerator.core import IterationMode, available_species, Structure, make_supercell
from sqsgenerator.io import read_structure_from_file
from sqsgenerator.settings.defaults import defaults, random_mode
from sqsgenerator.settings.exceptions import BadSettings
from sqsgenerator.settings.functional import parameter as parameter_, if_, isa, star

__parameter_registry = collections.OrderedDict({})

# the parameter decorator registers all the "processor" function with their names in __parameter_registry
# the ordering will be according to their definition in this file
parameter = partial(parameter_, registry=__parameter_registry)


def int_safe(x):
    return int(float(x))


def convert(o, to=int, converter=None, on_fail=BadSettings, message=None):
    if isinstance(o, to): return o
    try:
        r = (to if converter is None else converter)(o)
    except (ValueError, TypeError):
        if on_fail is not None:
            raise on_fail(message) if message is not None else on_fail()
    else:
        return r


@parameter('atol', default=defaults.atol)
def read_atol(settings: AttrDict):
    if not isinstance(settings.atol, float) or settings.atol < 0:
        raise BadSettings(f'The absolute tolerance can be only a positive floating point number')
    return settings.atol


@parameter('rtol', default=defaults.rtol)
def read_rtol(settings: AttrDict):
    if not isinstance(settings.rtol, float) or settings.rtol < 0:
        raise BadSettings(f'The relative tolerance can be only a positive floating point number')
    return settings.rtol


@parameter('mode', default=defaults.mode, required=True)
def read_mode(settings: AttrDict):
    if isinstance(settings.mode, IterationMode):
        return settings.mode
    if settings.mode not in IterationMode.names:
        raise BadSettings(f'Unknown iteration mode "{settings.mode}". '
                          f'Available iteration modes are {list(IterationMode.names.keys())}')
    return IterationMode.names[settings.mode]


@parameter('iterations', default=defaults.iterations, required=if_(random_mode)(True)(False))
def read_iterations(settings: AttrDict):
    num_iterations = convert(settings.iterations, converter=int_safe,
                             message=f'Cannot convert "{settings.iterations}" to int')
    if num_iterations < 0: raise BadSettings('"iterations" must be positive')
    return num_iterations


@parameter('max_output_configurations', default=defaults.max_output_configurations)
def read_max_output_configurations(settings: AttrDict):
    num_confs = convert(settings.max_output_configurations, converter=int_safe,
                        message=f'Cannot convert "{settings.max_output_configurations}" to int')
    if num_confs < 0:
        raise BadSettings('"max_output_configurations" must be positive')
    return num_confs


@parameter('structure')
def read_structure(settings: AttrDict) -> Structure:
    needed_fields = {'lattice', 'coords', 'species'}
    s = settings.structure
    structure = None
    if isinstance(s, Structure):
        structure = s

    if have_feature(Feature.ase) and structure is None:
        from ase import Atoms
        if isinstance(s, Atoms):
            structure = from_ase_atoms(s)
    if have_feature(Feature.pymatgen) and structure is None:
        from pymatgen.core import Structure as PymatgenStructure, PeriodicSite
        if isinstance(s, PymatgenStructure):
            structure = from_pymatgen_structure(s)
        elif isinstance(s, collections.abc.Iterable):
            site_list = list(s)
            if site_list and all(map(isa(PeriodicSite), site_list)):
                structure = from_pymatgen_structure(PymatgenStructure.from_sites(site_list))

    if isinstance(s, dict) and structure is None:
        if 'file' in s:
            structure = read_structure_from_file(settings)
        elif all(field in s for field in needed_fields):
            lattice = np.array(s.lattice)
            coords = np.array(s.coords)
            species = list(s.species)
            structure = Structure(lattice, coords, species, (True, True, True))
        else:
            raise BadSettings(f'A structure dictionary needs the following fields {needed_fields}')

    if structure is None: raise BadSettings(f'Cannot read structure from the settings, "{type(s)}"')
    if isinstance(s, dict) and 'supercell' in s:
        sizes = settings.structure.supercell
        if len(sizes) != 3: raise BadSettings('To create a supercell you need to specify three lengths')
        structure = make_supercell(structure, *sizes)
        del settings.structure['supercell']

    return structure


@parameter('which', default=defaults.which, required=True)
def read_which(settings: AttrDict):
    structure = settings.structure
    if isinstance(settings.which, str):
        sublattice = settings.which
        allowed_sublattices = {'all', }.union(structure.unique_species)
        if sublattice not in allowed_sublattices:
            raise BadSettings(f'The structure does not have an "{sublattice}" sublattice. '
                              f'Possible values would be {allowed_sublattices}')
        if sublattice == 'all':
            mask = tuple(range(structure.num_atoms))
        else:
            mask = tuple(i for i, sp in enumerate(structure.species) if sp.symbol == sublattice)
        which = mask
    elif isinstance(settings.which, (list, tuple)):
        sublattice = tuple(set(settings.which))  # eliminate duplicates
        if len(sublattice) < 2:
            raise BadSettings('You need to at least specify two different lattice positions to define a sublattice')
        if not all(map(isa(int), sublattice)):
            raise BadSettings(f'I do only understand integer lists to specify a sublattice')
        if not all(map(lambda _: 0 <= _ < structure.num_atoms, sublattice)):
            raise BadSettings(f'All indices in the list must be 0 <= index < {structure.num_atoms}')
        which = tuple(settings.which)
    else:
        raise BadSettings('I do not understand your composition specification')
    # we remove the which clause from the settings, since we are replacing the structure with the sublattice structure
    return which


@parameter('composition', default=defaults.composition, required=True)
def read_composition(settings: AttrDict):
    which = settings.which

    if not isinstance(settings.composition, dict):
        raise BadSettings(f'Cannot interpret "composition" setting. I expect a dictionary')

    allowed_symbols = set(map(attr('symbol'), available_species()))
    for species, amount in settings.composition.items():
        if species not in allowed_symbols:
            raise BadSettings(
                f'I have never heard of the chemical element "{species}". Please use a real chemical element!')
        if not isinstance(amount, int) or amount < 1:
            raise BadSettings(f'I can only distribute an integer number of atoms '
                              f'on the "{species}" sublattice. You specified "{amount}"')

    num_atoms_on_sublattice = len(which)
    num_distributed_atoms = sum(settings.composition.values())
    if num_distributed_atoms != num_atoms_on_sublattice:
        raise BadSettings(f'The sublattice has {num_atoms_on_sublattice} '
                          f'but you tried to distribute {num_distributed_atoms} atoms')

    species = list(chain(*map(star(repeat), settings.composition.items())))
    return species


@parameter('shell_distances', default=defaults.shell_distances, required=True)
def read_shell_distances(settings: AttrDict):
    if not isinstance(settings.shell_distances, (list, tuple, set)):
        raise BadSettings('I only understand list, sets, and tuples for the shell-distances parameter')
    if not all(map(isa(numbers.Real), settings.shell_distances)):
        raise BadSettings('All shell distances must be real values')
    distances = list(filter(lambda d: not np.isclose(d, 0.0), settings.shell_distances))
    if len(distances) < 1:
        raise BadSettings('You need to specify at least one shell-distance')
    for distance in distances:
        if distance < 0.0:
            raise BadSettings(f'A distance can never be less than zero. You specified "{distance}"')
    sorted_distances = list(sorted(distances))
    sorted_distances.insert(0, 0.0)
    return sorted_distances


@parameter('shell_weights', default=defaults.shell_weights, required=True)
def read_shell_weights(settings: AttrDict):
    if not isinstance(settings.shell_weights, dict):
        raise BadSettings('I only understand dictionaries for the shell-weight parameter')
    if len(settings.shell_weights) < 1:
        raise BadSettings('You have to include at least one coordination shell to carry out the optimization')
    allowed_indices = set(range(1, len(settings.shell_distances)))

    parsed_weights = {
        convert(shell, to=int, message=f'A shell must be an integer. You specified {shell}'): \
            convert(weight, to=float, message=f'A weight must be a floating point number. You specified {weight}')
        for shell, weight in settings.shell_weights.items()
    }

    for shell in parsed_weights.keys():
        if shell not in allowed_indices:
            raise BadSettings(f'The shell {shell} you specified is not allowed. Allowed values are {allowed_indices}')
    return settings.shell_weights


@parameter('pair_weights', default=defaults.pair_weights, required=True)
def read_pair_weights(settings: AttrDict):
    nums = settings.structure.num_unique_species
    if isinstance(settings.pair_weights, (list, tuple, np.ndarray)):
        w = np.array(settings.pair_weights).astype(float)
        if w.shape != (nums, nums):
            raise BadSettings(
                f'The "pair_weights" you have specified has a wrong shape ({w.shape}). Expected {(nums, nums)}')
        if not np.allclose(w, w.T):
            raise BadSettings(f'The "pair_weights" matrix must be symmetric')
        return w
    else:
        raise BadSettings(f'As "pair_weights" I do expect a {nums}x{nums} matrix, '
                          f'since your structure contains {nums} different species')


@parameter('target_objective', default=defaults.target_objective, required=True)
def read_target_objective(settings: AttrDict):
    nums = settings.structure.num_unique_species
    nshells = len(settings.shell_weights)
    if isinstance(settings.target_objective, (int, float)):
        o = np.ones((nshells, nums, nums)).astype(float)
        for index, (shell_index, shell_weight) in enumerate(sorted(settings.shell_weights.items(), key=item(0))):
            o[index, :, :] = settings.target_objective * shell_weight
        return o
    elif isinstance(settings.target_objective, (list, tuple, np.ndarray)):
        o = np.array(settings.target_objective).astype(float)
        if o.ndim == 3:
            expected_shape = (nshells, nums, nums)
            if o.shape != expected_shape:
                raise BadSettings(f'The 3D "target_objective" you have specified '
                                  f'has a wrong shape ({o.shape}). Expected {expected_shape}')
            for i, shell_objective in enumerate(o, start=1):
                if not np.allclose(shell_objective, shell_objective.T):
                    raise BadSettings(f'The "target_objective" parameters in shell {i} are not symmetric')
            return o
        elif o.ndim == 2:
            expected_shape = (nums, nums)
            if o.shape != expected_shape:
                raise BadSettings(f'The 2D "target_objective" you have specified '
                                  f'has a wrong shape ({o.shape}). Expected {expected_shape}')
            if not np.allclose(o, o.T):
                raise BadSettings(f'The "target_objective" parameters are not symmetric')
            f = np.ones((nshells, nums, nums)).astype(float)
            for index, (shell_index, shell_weight) in enumerate(sorted(settings.shell_weights.items(), key=item(0))):
                f[index, :, :] = o * shell_weight
            return f
        else:
            raise BadSettings(f'The "target_objective" you have specified has a {o.ndim} dimensions. '
                              f'I only can cope with 2 and 3')
    else:
        raise BadSettings(f'Cannot interpret "target_objective" setting. '
                          f'Acceptable values are a single number, {nums}x{nums} or {nshells}x{nums}x{nums} matrices!')


@parameter('threads_per_rank', default=defaults.threads_per_rank, required=True)
def read_threads_per_rank(settings: AttrDict):
    converter = partial(convert, to=int, converter=int_safe, message='Cannot parse "threads_per_rank" argument')
    if isinstance(settings.threads_per_rank, (float, int)):
        return [converter(settings.threads_per_rank)]
    if isinstance(settings.threads_per_rank, (list, tuple, np.ndarray)):
        if len(settings.threads_per_rank) != 1:
            if not have_mpi_support():
                raise BadSettings(f'The module sqsgenerator.core.iteration was not compiled with MPI support')
        return list(map(converter, settings.threads_per_rank))

    raise BadSettings(f'Cannot interpret "threads_per_rank" setting.')


def process_settings(settings: AttrDict, params: T.Optional[T.Set[str]] = None, ignore=()):
    params = params if params is not None else set(parameter_list())
    last_needed_parameter = max(params, key=parameter_index)
    ignore = set(ignore)
    for index, (param, processor) in enumerate(__parameter_registry.items()):
        if param not in params:
            # we can only skip this parameter if None of the other parameters depends on param
            if parameter_index(param) > parameter_index(last_needed_parameter): continue
        if param in ignore: continue
        settings[param] = processor(settings)
    return settings


def parameter_list() -> T.List[str]:
    return list(__parameter_registry.keys())


def parameter_index(parameter: str) -> int:
    return parameter_list().index(parameter)
