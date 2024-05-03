
import typing as T

from sqsgenerator.fallback.attrdict import AttrDict
from sqsgenerator.settings.defaults import defaults
from sqsgenerator.settings.utils import build_structure
from sqsgenerator.settings.exceptions import BadSettings
from sqsgenerator.core import IterationSettings, IterationMode
from sqsgenerator.settings.readers import process_settings, parameter_list, to_internal_composition_specs


def construct_settings(settings: AttrDict, process: T.Optional[bool] = True, **overloads) -> IterationSettings:
    """
    Constructs a ``sqsgenerator.core.IterationSettings`` object. This object is needed to pass the {settings} to the
    C++ extension. This function does not modify the passed {settings} but creates a copy of it. This function is meant
    for **internal use**

    :param settings: the dict-like settings object. The parameter is passed on to the ``AttrDict`` constructor
    :type settings: AttrDict
    :param process: process the settings (default is ``True``)
    :type process: bool
    :param overloads: keyword args are used to **overload** key in the {settings} dictionary
    :return: the ``sqsgenerator.core.IterationSettings`` object, ready to pass on to ``pair_analysis``
    :rtype: IterationsSettings
    :raises KeyError: if a key was passed in {overloads} which is not present in {settings}
    """

    settings = AttrDict(settings.copy())
    for overload, value in overloads.items():
        if overload not in settings:
            raise KeyError(overload)
        settings[overload] = value
    settings = process_settings(settings) if process else settings
    try:
        return IterationSettings(
            settings.structure,
            to_internal_composition_specs(settings.composition, settings.structure),
            settings.target_objective,
            settings.pair_weights,
            settings.prefactors,
            dict(settings.shell_weights),
            settings.iterations if settings.mode == IterationMode.random else -1,
            settings.max_output_configurations,
            list(settings.shell_distances),
            list(settings.threads_per_rank),
            settings.atol,
            settings.rtol,
            settings.mode,
            dict(settings.callbacks)
        )
    except (ValueError, RuntimeError) as e:
        raise BadSettings(e)


__all__ = [
    'BadSettings',
    'parameter_list',
    'process_settings',
    'construct_settings',
    'defaults',
    'build_structure'
]
