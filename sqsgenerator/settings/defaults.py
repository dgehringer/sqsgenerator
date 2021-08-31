
import numpy as np
from attrdict import AttrDict
from operator import itemgetter as item
from sqsgenerator.core import IterationMode, Structure, default_shell_distances as default_shell_distances_core
from sqsgenerator.settings.functional import const, if_


ATOL = 1e-3
RTOL = 1e-5


def num_shells(settings: AttrDict):
    return len(settings.shell_weights)


def num_species(settings: AttrDict):
    return settings.structure.slice_with_species(settings.composition, settings.which).num_unique_species


def random_mode(settings) -> bool:
    return settings.mode == IterationMode.random


def default_which(settings: AttrDict):
    structure: Structure = settings.structure
    return tuple(range(structure.num_atoms))


def default_composition(settings: AttrDict):
    structure: Structure = settings.structure
    which = settings.which
    return structure[which].symbols.tolist()


def default_shell_distances(settings: AttrDict):
    structure = settings.structure.slice_with_species(settings.composition, settings.which)
    return default_shell_distances_core(structure, settings.atol, settings.rtol)


def default_shell_weights(settings: AttrDict):
    return {i: 1.0/i for i, distance in enumerate(settings.shell_distances[1:], start=1)}


def default_pair_weights(settings: AttrDict):
    structure = settings.structure.slice_with_species(settings.composition, settings.which)
    per_shell_weights = (~np.eye(structure.num_unique_species, dtype=bool)).astype(float)
    stack = [weight*per_shell_weights for _, weight in sorted(settings.shell_weights.items(), key=item(0))]
    return np.stack(stack)


def default_target_objective(settings: AttrDict):
    structure = settings.structure.slice_with_species(settings.composition, settings.which)
    return np.zeros((num_shells(settings), structure.num_unique_species, structure.num_unique_species))


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
        threads_per_rank=const([-1])
    )
)