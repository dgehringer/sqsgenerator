import attrdict
from .exceptions import BadSettings
from sqsgenerator.core import IterationSettings
from .readers import process_settings, parameter_list


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

__all__ = [
    'BadSettings',
    'parameter_list',
    'process_settings',
    'construct_settings',
]
