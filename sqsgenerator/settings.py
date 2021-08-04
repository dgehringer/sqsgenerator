import pprint

import yaml
import attrdict
import numpy as np
import collections
from sqsgenerator.core import IterationMode, default_shell_distances, BadSettings, available_species
from sqsgenerator.core.fn import parameter as parameter_, partial, if_, item, attr, identity, method
from sqsgenerator.structure import Structure, make_supercell, from_ase_atoms, from_pymatgen_structure, num_species, num_sites_on_sublattice, unique_species, read_structure_from_file, structure_to_dict
from sqsgenerator.compat import Feature, have_mpi_support


__parameter_registry = collections.OrderedDict({})


parameter = partial(parameter_, registry=__parameter_registry)

ATOL = 1e-5
RTOL = 1e-8


def num_shells(settings): return len(settings.shell_weights)


def random_mode(settings): return settings.mode == IterationMode.random


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


@parameter('pair_weights', default=lambda s: np.ones((num_species(s.structure), num_species(s.structure))), required=True)
def read_pair_weights(settings: attrdict.AttrDict):
    nums = num_species(settings.structure)
    if isinstance(settings.pair_weights, (list, tuple)):
        w = np.array(settings.pair_weights).astype(float)
        if w.shape == (nums, nums): return w
        else: raise BadSettings(f'The "pair_weights" you have specified has a wrong shape ({w.shape}). Expected {(nums, nums)}')
    else:
        raise BadSettings(f'As "pair_weights" I do expect a {nums}x{nums} matrix, since your structure contains {nums} different species')


@parameter('target_objective', default=lambda s: np.zeros((num_shells(s), num_species(s.structure), num_species(s.structure))), required=True)
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


@parameter('composition', required=True)
def read_composition(settings: attrdict.AttrDict):
    if not isinstance(settings.composition, dict): raise BadSettings(f'Cannot interpret "composition" setting. I expected a dictionary')
    allowed_sublattices = {'all',}.union(unique_species(settings.structure))
    allowed_symbols = set(map(attr('symbol'), available_species()))
    for sublattice in settings.composition.keys():
        if sublattice not in allowed_sublattices: raise BadSettings(f'The structure does not have an "{sublattice}" subllatice. Possible values would be {allowed_sublattices}')
        num_sites = num_sites_on_sublattice(settings.structure, sublattice)
        sublattice_composition = settings.composition[sublattice]
        if not isinstance(sublattice_composition, dict): raise BadSettings(f'Cannot interpret "composition" setting, specified for sublattice {sublattice}')
        for species, number in sublattice_composition.items():
            if species not in allowed_symbols: raise BadSettings(f'I have never heard of the chemical element "{species}". Please use a real chemical element!')
            if not isinstance(number, (float, int)): raise BadSettings(f'I do not understand how many atoms of type "{species}" I should distribute on the "{sublattice}" sublattice. You said "{number}"')
            sublattice_composition[species] = int(number)
        num_distributed_atoms = sum(sublattice_composition.values())
        if num_distributed_atoms != num_sites: raise BadSettings(f'Cannot distribute {num_distributed_atoms} on the "{sublattice}" sublattice, which has actually {num_sites} sites')
    return settings.composition


def process_settings(settings: attrdict.AttrDict):
    for param, processor in __parameter_registry.items():
        settings[param] = processor(settings)
    return settings


def settings_to_dict(settings: attrdict.AttrDict):
    converters = {
        int: identity,
        float: identity,
        str: identity,
        Structure: structure_to_dict,
        IterationMode: str,
        np.ndarray: method('tolist')
    }

    def _generic_to_dict(d):
        td = type(d)
        if isinstance(d, (tuple, list, set)):
            return td(map(_generic_to_dict, d))
        elif isinstance(d, dict):
            return dict( { k: _generic_to_dict(v) for k, v in d.items() } )
        elif td in converters:
            return converters[td](d)
        else: raise TypeError(f'No converter specified for type "{td}"')

    out = _generic_to_dict(settings)
    return out



if __name__ == '__main__':
    import compat
    # print(os.getcwd())
    d = attrdict.AttrDict(yaml.safe_load(open('examples/cs-cl.sqs.yaml')))
    # print(compat.have_ase(), compat.have_pymatgen(), compat.have_pyiron(), compat.have_mpi4py())
    import sqsgenerator.core.iteration
    print(sqsgenerator.core.iteration.__version__)
    proc = process_settings(d)
    s1 = settings_to_dict(proc).copy()
    s2 = settings_to_dict(process_settings(attrdict.AttrDict(s1))).copy()

    with open('proc1.yaml', 'w') as h: yaml.safe_dump(s1, h, default_flow_style=None)
    with open('proc2.yaml', 'w') as h: yaml.safe_dump(s2, h, default_flow_style=None)