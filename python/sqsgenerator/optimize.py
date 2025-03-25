from typing import Dict, Any, Union
from .core import (
    parse_config,
    optimize as _optimize,
    SqsConfiguration,
    SqsConfigurationFloat,
    SqsConfigurationDouble,
    SqsResultPack,
    LogLevel,
)


def optimize(
    config: Union[Dict[str, Any], SqsConfiguration], level: LogLevel = LogLevel.info
) -> SqsResultPack:
    c = (
        config
        if isinstance(config, (SqsConfigurationFloat, SqsConfigurationDouble))
        else parse_config(config)
    )
    return _optimize(c, level)
