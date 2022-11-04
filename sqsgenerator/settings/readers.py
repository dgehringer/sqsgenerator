"""
Module containing all the routines to process input settings, from the dict-like configuration. Those routines are
intended for internal use only, and are implicitly exported by the ``process_settings`` function.
Each of the function with a @parameter decorator are registered in the private module level {__parameter_registry} 
dict, which makes the visible to the ``process_settings`` function
"""

import numbers
import numpy as np
import typing as T
import collections
import collections.abc
from functools import partial
from operator import itemgetter as item
from sqsgenerator.fallback.attrdict import AttrDict
from sqsgenerator.io import read_structure_from_file
from sqsgenerator.settings.exceptions import BadSettings
from sqsgenerator.core import IterationMode, Structure, make_supercell
from sqsgenerator.compat import Feature, have_mpi_support, have_feature
from sqsgenerator.adapters import from_ase_atoms, from_pymatgen_structure
from sqsgenerator.settings.functional import parameter as parameter_, if_, isa
from sqsgenerator.settings.defaults import defaults, random_mode, num_shells, num_species
from sqsgenerator.settings.utils import ensure_array_shape, ensure_array_symmetric, convert, int_safe, \
    build_structure, to_internal_composition_specs

__parameter_registry = collections.OrderedDict({})

# the parameter decorator registers all the "processor" function with their names in __parameter_registry
# the ordering will be according to their definition in this file
parameter = partial(parameter_, registry=__parameter_registry)


@parameter('bin_width', default=defaults.peak_isolation)
def read_bin_width(settings: AttrDict):
    if not isinstance(settings.bin_width, float) or settings.bin_width <= 0.0:
        raise BadSettings('The bin_width can be only a positive floating point number greater than 0.0')
    return settings.bin_width

@parameter('peak_isolation', default=defaults.peak_isolation)
def read_peak_isolation(settings: AttrDict):
    if not isinstance(settings.peak_isolation, float) or 0 <= settings.peak_isolation <= 1:
        raise BadSettings('The peak isolation in the pair histogram can be only a positive floating point number between 0 and 1')
    return settings.peak_isolation

@parameter('atol', default=defaults.atol)
def read_atol(settings: AttrDict):
    if not isinstance(settings.atol, float) or settings.atol < 0:
        raise BadSettings('The absolute tolerance can be only a positive floating point number')
    return settings.atol


@parameter('rtol', default=defaults.rtol)
def read_rtol(settings: AttrDict):
    if not isinstance(settings.rtol, float) or settings.rtol < 0:
        raise BadSettings('The relative tolerance can be only a positive floating point number')
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
    if num_iterations < 0:
        raise BadSettings('"iterations" must be positive')
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
            lattice = np.array(s['lattice'])
            coords = np.array(s['coords'])
            species = list(s['species'])
            structure = Structure(lattice, coords, species, (True, True, True))
        else:
            raise BadSettings(f'A structure dictionary needs the following fields {needed_fields}')

    if structure is None:
        raise BadSettings(f'Cannot read structure from the settings, "{type(s)}"')
    if isinstance(s, dict) and 'supercell' in s:
        sizes = settings.structure.supercell
        if len(sizes) != 3:
            raise BadSettings('To create a supercell you need to specify three lengths')
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
            raise BadSettings('I do only understand integer lists to specify a sublattice')
        if not all(map(lambda _: 0 <= _ < structure.num_atoms, sublattice)):
            raise BadSettings(f'All indices in the list must be 0 <= index < {structure.num_atoms}')
        which = tuple(settings.which)
    else:
        raise BadSettings('I do not understand your composition specification')
    # we remove the which clause from the settings, since we are replacing the structure with the sublattice structure
    return which


@parameter('composition', default=defaults.composition, required=True)
def read_composition(settings: AttrDict):
    structure = settings.structure[settings.which]
    if not isinstance(settings.composition, dict):
        raise BadSettings('Cannot interpret "composition" settings. I expect a dictionary')

    build_structure(settings.composition, structure)
    return settings.composition


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
        convert(shell, to=int, message=f'A shell must be an integer. You specified {shell}'):
            convert(weight, to=float, message=f'A weight must be a floating point number. You specified {weight}')
        for shell, weight in settings.shell_weights.items()
    }

    for shell in parsed_weights.keys():
        if shell not in allowed_indices:
            raise BadSettings(f'The shell {shell} you specified is not allowed. Allowed values are {allowed_indices}')
    return settings.shell_weights


def read_parameter_shaped_array(settings: AttrDict, parameter_name: str, constructor: T.Callable[[np.ndarray], np.ndarray]):
    nums = num_species(settings)
    nshells = num_shells(settings)

    if isinstance(settings.get(parameter_name), (list, tuple, np.ndarray)):
        w = np.array(settings.get(parameter_name)).astype(float)
        if w.ndim in {2, 3}:
            expected_shape = (nshells, nums, nums) if w.ndim == 3 else (nums, nums)

            ensure_array_shape(w, expected_shape, f'The 3D "{parameter_name}" you have specified '
                                                  f'has a wrong shape ({w.shape}). Expected {expected_shape}')
            ensure_array_symmetric(w, f'The "{parameter_name}" parameters are not symmetric')
            return constructor(w)

    raise BadSettings(f'As "{parameter_name}" I do expect a {nums}x{nums} matrix, '
                      f'since your structure contains {nums} different species')


@parameter('pair_weights', default=defaults.pair_weights, required=True)
def read_pair_weights(settings: AttrDict):
    sorted_weights = sorted(settings.shell_weights.items(), key=item(0))

    return read_parameter_shaped_array(settings, 'pair_weights',
                                       lambda w: w if w.ndim == 3
                                       else np.stack([w * shell_weight for _, shell_weight in sorted_weights]))


@parameter('target_objective', default=defaults.target_objective, required=True)
def read_target_objective(settings: AttrDict):
    if isinstance(settings.target_objective, (int, float)):
        target_objective = np.ones((num_shells(settings), num_species(settings), num_species(settings))) * settings.target_objective
        settings['target_objective'] = target_objective
    return read_parameter_shaped_array(settings, 'target_objective',
                                       lambda w: w if w.ndim == 3 else np.stack([w] * num_shells(settings)))


@parameter('prefactor_mode', default=defaults.prefactor_mode, required=True)
def read_prefactor_mode(settings: AttrDict):
    if not isinstance(settings.prefactor_mode, str):
        raise BadSettings('"prefactor_mode" must be of type str')
    if settings.prefactor_mode.lower() not in {'set', 'mul'}:
        raise BadSettings('"prefactor_mode" must be either {\'set\', \'mul\'}')
    return settings.prefactor_mode.lower()


@parameter('prefactors', default=defaults.prefactors, required=True)
def read_prefactors(settings: AttrDict):
    if isinstance(settings.prefactors, (int, float)):
        prefactors = np.ones((num_shells(settings), num_species(settings), num_species(settings))) * settings.prefactors
        settings['prefactors'] = prefactors
    prefactors = read_parameter_shaped_array(settings, 'prefactors',
                                       lambda w: w if w.ndim == 3 else np.stack([w] * num_shells(settings)))

    prefactors = prefactors if settings.prefactor_mode == 'set' else (prefactors * defaults.prefactors(settings))
    if np.any(np.isclose(prefactors, 0)):
        raise BadSettings('A prefactor of zero is not allowed as divison would lead to infinite numbers')
    return prefactors


@parameter('threads_per_rank', default=defaults.threads_per_rank, required=True)
def read_threads_per_rank(settings: AttrDict):
    converter = partial(convert, to=int, converter=int_safe, message='Cannot parse "threads_per_rank" argument')
    if isinstance(settings.threads_per_rank, (float, int)):
        return [converter(settings.threads_per_rank)]
    if isinstance(settings.threads_per_rank, (list, tuple, np.ndarray)):
        if len(settings.threads_per_rank) != 1:
            if not have_mpi_support():
                raise BadSettings('The module sqsgenerator.core.iteration was not compiled with MPI support')
        return list(map(converter, settings.threads_per_rank))

    raise BadSettings('Cannot interpret "threads_per_rank" setting.')


def process_settings(settings: AttrDict, params: T.Optional[T.Set[str]] = None, ignore: T.Iterable[str]=()) -> AttrDict:
    """
    Process an dict-like input parameters, according to the rules specified in the 
    `Input parameter documentation <https://sqsgenerator.readthedocs.io/en/latest/input_parameters.html>`_. This function
    should be used for processing user input. Therefore, exports the parser functions defined in 
    ``sqsgenerator.settings.readers``. To specify a specify subset of parameters the {params} argument is used.
    To {ignore} specific parameters pass a list of parameter names

    :param settings: the dict-like user configuration
    :type settings: AttrDict
    :param params: If specified only the subset of {params} is processed (default is ``None``)
    :type params: Optional[Set, None]
    :param ignore: a list/iterable of params to ignore (default is ``()``)
    :type ignore: Iterable[str]
    :return: the processed settings dictionary
    :rtype: AttrDict
    """
    
    params = params if params is not None else set(parameter_list())
    last_needed_parameter = max(params, key=parameter_index)
    ignore = set(ignore)
    for index, (param, processor) in enumerate(__parameter_registry.items()):
        if param not in params:
            # we can only skip this parameter if None of the other parameters depends on param
            if parameter_index(param) > parameter_index(last_needed_parameter):
                continue
        if param in ignore:
            continue
        settings[param] = processor(settings)
    return settings


def parameter_list() -> T.List[str]:
    return list(__parameter_registry.keys())


def parameter_index(p: str) -> int:
    return parameter_list().index(p)
