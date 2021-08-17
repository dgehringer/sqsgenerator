
import numbers
import attrdict
import numpy as np
import collections
import typing as T
from operator import attrgetter as attr, itemgetter as item, methodcaller as method
from functools import partial
from itertools import repeat, chain
from .exceptions import BadSettings
from .functional import parameter as parameter_, if_, isa, identity
from sqsgenerator.core import IterationMode, default_shell_distances, available_species, IterationSettings, Structure, structure_to_dict, make_supercell
from sqsgenerator.settings.structure import from_ase_atoms, from_pymatgen_structure, num_species, read_structure_from_file
from sqsgenerator.compat import Feature, have_mpi_support, have_feature


__parameter_registry = collections.OrderedDict({})

# the parameter decorator registers all the "processor" function with their names in __parameter_registry
# the ordering will be according to their definition in this file
parameter = partial(parameter_, registry=__parameter_registry)

ATOL = 1e-3
RTOL = 1e-5


def num_shells(settings): return len(settings.shell_weights)


def random_mode(settings): return settings.mode == IterationMode.random


def convert(o, to=int, on_fail=BadSettings, message=None):
    if isinstance(o, to): return o
    try:
        r = to(o)
    except ValueError:
        if on_fail is not None:
            raise on_fail(message) if message is not None else on_fail()
    else:
        return r


@parameter('atol', default=ATOL)
def read_atol(settings : attrdict.AttrDict):
    if not isinstance(settings.atol, float) or settings.atol < 0: raise BadSettings(f'The absolute tolerance can be only a positive floating point number')
    return settings.atol


@parameter('rtol', default=RTOL)
def read_atol(settings : attrdict.AttrDict):
    if not isinstance(settings.atol, float) or settings.atol < 0: raise BadSettings(f'The relative tolerance can be only a positive floating point number')
    return settings.rtol


@parameter('composition', required=True)
def read_composition(settings: attrdict.AttrDict):
    structure = read_structure(settings)
    if not isinstance(settings.composition, dict): raise BadSettings(f'Cannot interpret "composition" setting. I expect a dictionary')
    allowed_symbols = set(map(attr('symbol'), available_species()))
    if 'which' in settings.composition:
        if isinstance(settings.composition.which, str):
            sublattice = settings.composition.which
            allowed_sublattices = {'all', }.union(structure.unique_species)
            if sublattice not in allowed_symbols: raise BadSettings(f'I have never heard of the chemical element "{sublattice}". Please use a real chemical element!')
            if sublattice not in allowed_sublattices: raise BadSettings(f'The structure does not have an "{sublattice}" sublattice. Possible values would be {allowed_sublattices}')
            if sublattice == 'all': mask = tuple(range(structure.num_atoms))
            else: mask = tuple(i for i, sp in enumerate(structure.species) if sp.symbol == sublattice)
            which = mask
        elif isinstance(settings.composition.which, (list, tuple)):
            sublattice = tuple(set(settings.composition.which)) # eliminate duplicates
            if len(sublattice) < 2: raise BadSettings('You need to at least specify two different lattice positions to define a sublattice')
            if not all(map(isa(int), sublattice)): raise BadSettings(f'I do only understand integer lists to specify a sublattice')
            if not all(map(lambda _: 0 <= _ < structure.num_atoms, sublattice)): raise BadSettings(f'All indices in the list must be 0 <= index < {structure.num_atoms}')
            which = settings.composition.which
        else: raise BadSettings('I do not understand your composition specification')
        # we remove the which clause from the settings, since we are replacing the structure with the sublattice structure
        del settings['composition']['which']
    else:
        # the which clause is not present
        which = tuple(range(structure.num_atoms))

    actual_composition = lambda: ((k, v) for k, v in settings.composition.items() if k not in {'which'})
    for species, amount in actual_composition():
        if species == 'which': continue
        if species not in allowed_symbols: raise BadSettings(f'I have never heard of the chemical element "{species}". Please use a real chemical element!')
        if not isinstance(amount, int) or amount < 1: raise BadSettings(f'I can only distribute an integer number of atoms on the "{species}" sublattice. You specified "{amount}"')

    num_atoms_on_sublattice = len(which)
    num_distributed_atoms = sum(amount for _, amount in actual_composition())
    if num_distributed_atoms != num_atoms_on_sublattice: raise BadSettings(f'The sublattice has {num_atoms_on_sublattice} but you tried to distribute {num_distributed_atoms} atoms')

    is_sublattice = tuple(range(structure.num_atoms)) != which
    if is_sublattice:
        settings['sublattice'] = dict(
            structure=structure,
            which=which
        )
        sublattice_structure = structure[which]
        species = list(chain(*(repeat(*c) for c in actual_composition())))
        settings['structure'] = Structure(sublattice_structure.lattice, sublattice_structure.frac_coords, species)

    settings['is_sublattice'] = is_sublattice
    return settings.composition


@parameter('structure')
def read_structure(settings : attrdict.AttrDict) -> Structure:
    needed_fields = {'lattice', 'coords', 'species'}
    s = settings.structure
    structure = None
    if isinstance(s, Structure): structure = s
    elif 'file' in s:
        structure = read_structure_from_file(settings)
    elif isinstance(s, dict):
        if all(field in s for field in needed_fields):
            lattice = np.array(s.lattice)
            coords = np.array(s.coords)
            species = list(s.species)
            structure = Structure(lattice, coords, species, (True, True, True))
        else: raise BadSettings(f'A structure dictionary needs the following fields {needed_fields}')

    if have_feature(Feature.ase) and structure is None:
        from ase import Atoms
        if isinstance(s, Atoms): structure = from_ase_atoms(s)
    if have_feature(Feature.pymatgen) and structure is None:
        from pymatgen.core import Structure as PymatgenStructure
        if isinstance(s, PymatgenStructure): structure = from_pymatgen_structure(s)
    if structure is None: raise BadSettings('Cannot read structure from the settings')

    if 'supercell' in s:
        sizes = settings.structure.supercell
        if len(sizes) != 3: raise BadSettings('To create a supercell you need to specify three lengths')
        structure = make_supercell(structure, *sizes)
        del settings.structure['supercell']
    return structure


@parameter('mode', default=IterationMode.random, required=True)
def read_mode(settings: attrdict.AttrDict):
    if settings.mode not in IterationMode.names:
        raise BadSettings(f'Unknown iteration mode "{settings.mode}". Available iteration modes are {list(IterationMode.names.keys())}')
    return IterationMode.names[settings.mode]


@parameter('iterations', default=if_(random_mode)(1e5)(-1), required=if_(random_mode)(True)(False))
def read_iterations(settings: attrdict.AttrDict):
    return int(float(settings.iterations))


@parameter('max_output_configurations', default=10)
def read_max_output_configurations(settings : attrdict.AttrDict):
    return settings.max_output_configurations


@parameter('shell_distances', default=lambda s: default_shell_distances(s.structure, s.atol, s.rtol), required=True)
def read_shell_distances(settings: attrdict.AttrDict):
    if not isinstance(settings.shell_distances, (list, tuple, set)): raise BadSettings('I only understand list, sets, and tuples for the shell-distances parameter')
    if not all(map(isa(numbers.Real), settings.shell_distances)): raise BadSettings('All shell distances must be real values')

    sorted_distances = list(sorted(settings.shell_distances))

    for distance in sorted_distances:
        if distance < 0.0: raise BadSettings(f'A distance can never be less than zero. You specified "{distance}"')
    return sorted_distances


@parameter('shell_weights', default=lambda settings: {i: 1.0/i for i, distance in enumerate(settings.shell_distances[1:], start=1)}, required=True)
def read_shell_weights(settings: attrdict.AttrDict):
    if not isinstance(settings.shell_weights, dict): raise BadSettings('I only understand dictionaries for the shell-weight parameter')
    allowed_indices = set(range(1, len(settings.shell_distances)))

    parsed_weights = {
            convert(shell, to=int, message=f'A shell must be an integer. You specified {shell}'):\
            convert(weight, to=float, message=f'A weight must be a floating point number. You specified {weight}')
            for shell, weight in settings.shell_weights.items()
        }

    for shell in parsed_weights.keys():
        if shell not in allowed_indices: raise BadSettings(f'The shell {shell} you specified is not allowed. Allowed values are {allowed_indices}')
    return settings.shell_weights


@parameter('pair_weights', default=lambda s: np.ones((num_species(s.structure), num_species(s.structure))), required=True)
def read_pair_weights(settings: attrdict.AttrDict):
    nums = num_species(settings.structure)
    if isinstance(settings.pair_weights, (list, tuple)):
        w = np.array(settings.pair_weights).astype(float)
        if w.shape == (nums, nums): return w
        else: raise BadSettings(f'The "pair_weights" you have specified has a wrong shape ({w.shape}). Expected {(nums, nums)}')
    else:
        raise BadSettings(f'As "pair_weights" I do expect a {nums}x{nums} matrix, since your structure contains {nums} different species')


@parameter('target_objective', default=lambda s: np.zeros((num_shells(s), s.structure.num_unique_species, s.structure.num_unique_species)), required=True)
def read_target_objective(settings: attrdict.AttrDict):
    nums = num_species(settings.structure)
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


def process_settings(settings: attrdict.AttrDict, params: T.Optional[T.Set[str]] = None):
    params = params if params is not None else set(parameter_list())
    last_needed_parameter = max(params, key=parameter_index)
    for index, (param, processor) in enumerate(__parameter_registry.items()):
        if param not in params:
            # we can only skip this parameter if None of the other parameters dpends on param
            if parameter_index(param) > parameter_index(last_needed_parameter): continue
        settings[param] = processor(settings)
    return settings


def parameter_list() -> T.List[str]:
    return list(__parameter_registry.keys())


def parameter_index(parameter: str) -> int:
    return parameter_list().index(parameter)