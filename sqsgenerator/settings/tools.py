
import attrdict
import numpy as np
import typing as T
from .functional import identity
from .readers import process_settings
from operator import methodcaller as method
from sqsgenerator.core import Structure, structure_to_dict, IterationMode, IterationSettings


def settings_to_dict(settings: attrdict.AttrDict) -> T.Dict[str, T.Any]:
    converters = {
        int: identity,
        float: identity,
        str: identity,
        bool: identity,
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


def construct_settings(settings: attrdict.AttrDict, process=True) -> IterationSettings:
    settings = process_settings(settings) if process else settings
    return IterationSettings(
        settings.structure,
        settings.target_objective,
        settings.pair_weights,
        dict(settings.shell_weights),
        settings.iterations,
        settings.max_output_configurations,
        list(settings.shell_distances),
        list(settings.threads_per_rank),
        settings.atol,
        settings.rtol,
        settings.mode
    )