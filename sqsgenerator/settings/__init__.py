
from attrdict import AttrDict

from sqsgenerator.core import IterationSettings
from sqsgenerator.settings.defaults import defaults
from sqsgenerator.settings.exceptions import BadSettings
from sqsgenerator.settings.readers import process_settings, parameter_list


def construct_settings(settings: AttrDict, process=True, **overloads) -> IterationSettings:
    settings = AttrDict(settings.copy())
    for overload, value in overloads.items():
        if overload not in settings:
            raise KeyError(overload)
        settings[overload] = value
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

__all__ = [
    'BadSettings',
    'parameter_list',
    'process_settings',
    'construct_settings',
]
