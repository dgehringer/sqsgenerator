import json
from typing import Any, Callable

from .core import (
    IterationMode,
    LogLevel,
    Prec,
    SqsConfiguration,
    SqsConfigurationDouble,
    SqsConfigurationFloat,
    SqsResultPack,
    SublatticeMode,
)
from .core import (
    optimize as _optimize,
)
from .core import (
    parse_config as _parse_config,
)

__all__ = ["optimize", "parse_config"]


def _parse_prec(string: str) -> Prec:
    """
    Parse a string into a Prec enum value.

    Args:
        string (str): The string to parse.

    Returns:
        Prec: The corresponding Prec enum value.
    """
    match string.lower():
        case "double":
            return Prec.double
        case "single":
            return Prec.single
        case _:
            raise ValueError(f"Invalid precision: {string}. Use 'double' or 'single'.")


def _parse_iteration_mode(string: str) -> IterationMode:
    """
    Parse a string into an IterationMode enum value.

    Args:
        string (str): The string to parse.

    Returns:
        IterationMode: The corresponding IterationMode enum value.
    """
    match string.lower():
        case "random":
            return IterationMode.random
        case "systematic":
            return IterationMode.systematic
        case _:
            raise ValueError(
                f"Invalid iteration mode: {string}. Use 'random' or 'systematic'."
            )


def _parse_sublattice_mode(string: str) -> SublatticeMode:
    """
    Parse a string into a SublatticeMode enum value.

    Args:
        string (str): The string to parse.

    Returns:
        SublatticeMode: The corresponding SublatticeMode enum value.
    """
    match string.lower():
        case "split":
            return SublatticeMode.split
        case "interact":
            return SublatticeMode.interact
        case _:
            raise ValueError(
                f"Invalid sublattice mode: {string}. Use 'split' or 'interact'."
            )


def parse_config(
    config: dict[str, Any] | str, inplace: bool = False
) -> SqsConfiguration:
    """
    Parse the configuration dictionary into a SqsConfiguration object.

    Args:
        config (dict[str, Any]): Configuration dictionary.
        inplace (bool, optional): If `True`, return a modified version of the input dictionary.

    Returns:
        SqsConfiguration: Parsed configuration object.
    """
    if isinstance(config, str):
        config = json.loads(config)
    config = config.copy() if not inplace else config

    def apply(key: str, f: Callable[[str], Any]) -> None:
        if key in config:
            config[key] = f(config[key])

    apply("prec", _parse_prec)
    apply("iteration_mode", _parse_iteration_mode)
    apply("sublattice_mode", _parse_sublattice_mode)

    return _parse_config(config)  # type: ignore[return-value]


def optimize(
    config: dict[str, Any] | SqsConfiguration, level: LogLevel = LogLevel.warn
) -> SqsResultPack:
    c = (
        config
        if isinstance(config, (SqsConfigurationFloat, SqsConfigurationDouble))
        else parse_config(config)
    )
    return _optimize(c, level)
