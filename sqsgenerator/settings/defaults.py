
import collections

import frozendict
import numpy as np
from math import isclose
from operator import itemgetter as item
from sqsgenerator.fallback.attrdict import AttrDict
from sqsgenerator.settings.functional import const, if_
from sqsgenerator.settings.utils import build_structure, to_internal_composition_specs
from sqsgenerator.core import IterationMode, Structure, compute_prefactors as default_compute_prefactors, ATOL, RTOL


DEFAULT_PAIR_HIST_PEAK_ISOLATION = 0.25
DEFAULT_PAIR_HIST_BIN_WIDTH = 0.15


DEFAULT_CALLBACKS = frozendict.frozendict(dict(
    found_better_or_equal=[],
    found_better=[]
))



def num_shells(settings: AttrDict):
    return len(settings.shell_weights)


def num_species(settings: AttrDict):
    return build_structure(settings.composition, settings.structure[settings.which]).num_unique_species


def random_mode(settings) -> bool:
    if "mode" in settings:
        return settings.mode == IterationMode.random
    # if there is no mode specified sqsgenerator runs in random (=Monte-Carlo9 mode
    return True

def default_which(settings: AttrDict):
    structure: Structure = settings.structure
    return tuple(range(structure.num_atoms))


def default_composition(settings: AttrDict):
    structure: Structure = settings.structure
    which = settings.which
    return dict(collections.Counter(structure[which].symbols.tolist()))


def default_shell_distances(settings: AttrDict):
    structure = settings.structure[settings.which].sorted()
    mask = ~np.eye(len(structure), dtype=bool)
    d2 = structure.distance_matrix[mask]

    max_dist = np.amax(d2)

    bin_width = settings.get('bin_width', DEFAULT_PAIR_HIST_BIN_WIDTH)
    peak_isolation = settings.get('peak_isolation', DEFAULT_PAIR_HIST_PEAK_ISOLATION)

    nbins = int(max_dist / bin_width)

    frequencies, edges = np.histogram(d2, bins=nbins+1, range=(0.0, max_dist + 2.0 * bin_width))
    lower_edges, upper_edges = edges[:-1], edges[1:]

    shells = []
    for i in range(1, len(frequencies)-1):
        prev_freq, freq, next_freq = frequencies[i-1:i+2]
        threshold = (1.0 - peak_isolation) * freq
        if threshold > prev_freq and threshold > next_freq:  # check if the surrounding bins are smallen then freq * (1-peak_isolation)
            upper_edge = upper_edges[i]
            max_element = np.amax(d2[d2 <= upper_edge])
            shells.append(max_element)

    if not any(isclose(0.0, shell, abs_tol=settings.atol, rel_tol=settings.rtol) for shell in shells):
        shells.insert(0, 0.0)

    return shells


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
                                      dict(settings.shell_weights), settings.shell_distances, settings.atol,
                                      settings.rtol)


defaults = AttrDict(
    dict(
        peak_isolation=const(DEFAULT_PAIR_HIST_PEAK_ISOLATION),
        bin_width=const(DEFAULT_PAIR_HIST_BIN_WIDTH),
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
        threads_per_rank=const([-1]),
        callbacks=const(DEFAULT_CALLBACKS)
    )
)