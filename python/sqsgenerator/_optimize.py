import json
import warnings
from typing import Any, Callable, Optional, Union

from ._adapters import read as _read
from .core import (
    IterationMode,
    LogLevel,
    Prec,
    SqsCallback,
    SqsConfiguration,
    SqsConfigurationDouble,
    SqsConfigurationFloat,
    SqsResultPack,
    Structure,
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
    if (prec := string.lower()) == "double":
        return Prec.double
    elif prec == "single":
        return Prec.single
    else:
        raise ValueError(f"Invalid precision: {string}. Use 'double' or 'single'.")


def _parse_iteration_mode(string: str) -> IterationMode:
    """
    Parse a string into an IterationMode enum value.

    Args:
        string (str): The string to parse.

    Returns:
        IterationMode: The corresponding IterationMode enum value.
    """
    if (mode := string.lower()) == "random":
        return IterationMode.random
    elif mode == "systematic":
        return IterationMode.systematic
    else:
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
    if (mode := string.lower()) == "split":
        return SublatticeMode.split
    elif mode == "interact":
        return SublatticeMode.interact
    else:
        raise ValueError(
            f"Invalid sublattice mode: {string}. Use 'split' or 'interact'."
        )


def _preprocess_structure(structure_config: dict[str, Any]) -> dict[str, Any]:
    """
    Preprocess the structure configuration dictionary inplace.

    Args:
        structure_config (dict[str, Any]): The structure configuration dictionary containing
            information about the structure, such as lattice, coordinates, species, or a file path.

    Returns:
        dict[str, Any]: The updated structure configuration dictionary with processed structure data.
    """
    if structure_file_path := structure_config.get("file"):
        if (
            "lattice" in structure_config
            and "coords" in structure_config
            and "species" in structure_config
        ):
            warnings.warn(
                f"Structure data provided in both file and inline format. Using file data from {structure_file_path}.",
                UserWarning,
                stacklevel=2,
            )
        structure: Structure = _read(structure_file_path)
        structure_config["lattice"] = structure.lattice.tolist()
        structure_config["coords"] = structure.frac_coords.tolist()
        structure_config["species"] = structure.species

    return structure_config


def parse_config(
    config: Union[dict[str, Any], str], inplace: bool = False
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

    if structure_config := config.get("structure"):
        _preprocess_structure(structure_config)

    def apply(key: str, f: Callable[[str], Any]) -> None:
        if key in config:
            config[key] = f(config[key])

    apply("prec", _parse_prec)
    apply("iteration_mode", _parse_iteration_mode)
    apply("sublattice_mode", _parse_sublattice_mode)

    return _parse_config(config)  # type: ignore[return-value]


def optimize(
    config: Union[dict[str, Any], SqsConfiguration, str],
    level: LogLevel = LogLevel.warn,
    callback: Optional[SqsCallback] = None,
) -> SqsResultPack:
    """
    Optimize the SQS configuration based on the provided parameters.

    Args:
        config (dict[str, Any] | SqsConfiguration | str): The configuration data to optimize.
        level (LogLevel): The logging level for the optimization process. Defaults to `LogLevel.warn`.
        callback (SqsCallback | None): A callback function to monitor the optimization progress. Defaults to `None`.

    Returns:
        SqsResultPack: The result of the optimization process.
    """
    c = (
        config
        if isinstance(config, (SqsConfigurationFloat, SqsConfigurationDouble))
        else parse_config(config)
    )
    return _optimize(c, log_level=level, callback=callback)
