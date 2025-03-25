from typing import Union

from ._core import (
    parse_config,
    Prec,
    single,
    double,
    IterationMode,
    systematic,
    random,
    SublatticeMode,
    split,
    interact,
    LogLevel,
    optimize,
    StructureFloat,
    StructureDouble,
    StructureFormat,
    SqsConfigurationFloat,
    SqsConfigurationDouble,
    SqsResultSplitFloat,
    SqsResultSplitDouble,
    SqsResultInteractFloat,
    SqsResultInteractDouble,
    SqsResultPackSplitFloat,
    SqsResultPackSplitDouble,
    SqsResultPackInteractFloat,
    SqsResultPackInteractDouble,
    __version__ as __core__version__,
)


__version__ = __core__version__

SqsConfiguration = Union[SqsConfigurationFloat, SqsConfigurationDouble]
Structure = Union[StructureFloat, StructureDouble]
SqsResultSplit = Union[SqsResultSplitFloat, SqsResultSplitDouble]
SqsResultInteract = Union[SqsResultInteractFloat, SqsResultInteractDouble]
SqsResultPackSplit = Union[SqsResultPackSplitFloat, SqsResultPackSplitDouble]
SqsResultPackInteract = Union[SqsResultPackInteractFloat, SqsResultPackInteractDouble]

SqsResult = Union[SqsResultInteract, SqsResultSplit]
SqsResultPack = Union[SqsResultPackInteract, SqsResultPackSplit]

__all__ = [
    "parse_config",
    "Prec",
    "single",
    "double",
    "IterationMode",
    "systematic",
    "random",
    "SublatticeMode",
    "interact",
    "split",
    "LogLevel",
    "optimize",
    "StructureFormat",
    "StructureFloat",
    "StructureDouble",
    "SqsConfigurationFloat",
    "SqsConfigurationDouble",
    "SqsConfiguration",
    "Structure",
    "SqsResultSplitFloat",
    "SqsResultSplitDouble",
    "SqsResultSplit",
    "SqsResultInteractFloat",
    "SqsResultInteractDouble",
    "SqsResultInteract",
    "SqsResult",
    "SqsResultPackSplitFloat",
    "SqsResultPackSplitDouble",
    "SqsResultSplit",
    "SqsResultPackInteractFloat",
    "SqsResultPackInteractDouble",
    "SqsResultPackInteract",
    "SqsResultPack",
    "__version__",
]
