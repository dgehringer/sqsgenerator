import collections
import numpy as np
from attrdict import AttrDict
from operator import itemgetter as item
from sqsgenerator.settings.utils import build_structure, to_internal_composition_specs
from sqsgenerator.settings.functional import const, if_
from sqsgenerator.core import IterationMode, Structure, default_shell_distances as default_shell_distances_core, \
    compute_prefactors as default_compute_prefactors


ATOL = 1e-3
RTOL = 1e-5


def num_shells(settings: AttrDict):
    return len(settings.shell_weights)


def num_species(settings: AttrDict):
    return build_structure(settings.composition, settings.structure[settings.which]).num_unique_species


def random_mode(settings) -> bool:
    return settings.mode == IterationMode.random


def default_which(settings: AttrDict):
    structure: Structure = settings.structure
    return tuple(range(structure.num_atoms))


def default_composition(settings: AttrDict):
    structure: Structure = settings.structure
    which = settings.which
    return dict(collections.Counter(structure[which].symbols.tolist()))


def default_shell_distances(settings: AttrDict):
    structure = settings.structure[settings.which].sorted()
    return default_shell_distances_core(structure, settings.atol, settings.rtol)


def default_shell_weights(settings: AttrDict):
    return {i: 1.0/i for i, distance in enumerate(settings.shell_distances[1:], start=1)}


def default_pair_weights(settings: AttrDict):
    structure = build_structure(settings.composition, settings.structure[settings.which])
    per_shell_weights = (~np.eye(structure.num_unique_species, dtype=bool)).astype(float) * 0.5
    stack = [weight*per_shell_weights for _, weight in sorted(settings.shell_weights.items(), key=item(0))]
    return np.stack(stack)


def default_target_objective(settings: AttrDict):
    structure = build_structure(settings.composition, settings.structure[settings.which])
    return np.zeros((num_shells(settings), structure.num_unique_species, structure.num_unique_species))


def default_prefactors(settings: AttrDict):
    internal_composition_spec = to_internal_composition_specs(settings.composition, settings.structure)
    return default_compute_prefactors(settings.structure[settings.which], internal_composition_spec,
                                      dict(settings.shell_weights), settings.atol, settings.rtol)


defaults = AttrDict(
    dict(
        atol=const(ATOL),
        rtol=const(RTOL),
        mode=const(IterationMode.random),
        iterations=if_(random_mode)(100000)(-1),
        max_output_configurations=const(10),
        which=default_which,
        composition=default_composition,
        shell_distances=default_shell_distances,
        shell_weights=default_shell_weights,
        pair_weights=default_pair_weights,
        target_objective=default_target_objective,
        prefactors=default_prefactors,
        prefactor_mode=const('set'),
        threads_per_rank=const([-1])
    )
)