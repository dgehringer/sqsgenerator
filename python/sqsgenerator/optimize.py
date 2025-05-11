from typing import Any

__all__ = ["optimize", "parse_config"]

from .core import (
    LogLevel,
    SqsConfiguration,
    SqsConfigurationDouble,
    SqsConfigurationFloat,
    SqsResultPack,
    parse_config,
)
from .core import (
    optimize as _optimize,
)


def optimize(
    config: dict[str, Any] | SqsConfiguration, level: LogLevel = LogLevel.warn
) -> SqsResultPack:
    c = (
        config
        if isinstance(config, (SqsConfigurationFloat, SqsConfigurationDouble))
        else parse_config(config)
    )
    return _optimize(c, level)
