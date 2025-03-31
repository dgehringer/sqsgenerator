from typing import Union

from ._core import (
    IterationMode,
    LogLevel,
    Prec,
    SqsConfigurationDouble,
    SqsConfigurationFloat,
    SqsResultInteractDouble,
    SqsResultInteractFloat,
    SqsResultPackInteractDouble,
    SqsResultPackInteractFloat,
    SqsResultPackSplitDouble,
    SqsResultPackSplitFloat,
    SqsResultSplitDouble,
    SqsResultSplitFloat,
    StructureDouble,
    StructureFloat,
    StructureFormat,
    SublatticeMode,
    double,
    interact,
    optimize,
    parse_config,
    random,
    single,
    split,
    systematic,
)
from ._core import (
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
    "IterationMode",
    "LogLevel",
    "Prec",
    "SqsConfiguration",
    "SqsConfigurationDouble",
    "SqsConfigurationFloat",
    "SqsResult",
    "SqsResultInteract",
    "SqsResultInteractDouble",
    "SqsResultInteractFloat",
    "SqsResultPack",
    "SqsResultPackInteract",
    "SqsResultPackInteractDouble",
    "SqsResultPackInteractFloat",
    "SqsResultPackSplitDouble",
    "SqsResultPackSplitFloat",
    "SqsResultSplit",
    "SqsResultSplit",
    "SqsResultSplitDouble",
    "SqsResultSplitFloat",
    "Structure",
    "StructureDouble",
    "StructureFloat",
    "StructureFormat",
    "SublatticeMode",
    "__version__",
    "double",
    "interact",
    "optimize",
    "parse_config",
    "random",
    "single",
    "split",
    "systematic",
]
