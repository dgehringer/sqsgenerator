from typing import Callable, Union

from ._core import (
    Atom,
    IterationMode,
    LogLevel,
    ParseError,
    Prec,
    SqsCallbackContextDouble,
    SqsCallbackContextFloat,
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
    Timing,
    double,
    interact,
    load_result_pack,
    optimize,
    parse_config,
    random,
    single,
    split,
    systematic,
)
from ._core import (
    __build__ as __core__build__,
)
from ._core import (
    __version__ as __core__version__,
)

__version__ = __core__version__

__build__ = __core__build__


SqsConfiguration = Union[SqsConfigurationFloat, SqsConfigurationDouble]
Structure = Union[StructureFloat, StructureDouble]
SqsResultSplit = Union[SqsResultSplitFloat, SqsResultSplitDouble]
SqsResultInteract = Union[SqsResultInteractFloat, SqsResultInteractDouble]
SqsResultPackSplit = Union[SqsResultPackSplitFloat, SqsResultPackSplitDouble]
SqsCallbackContext = Union[SqsCallbackContextFloat, SqsCallbackContextDouble]
SqsResultPackInteract = Union[SqsResultPackInteractFloat, SqsResultPackInteractDouble]

SqsCallback = Callable[[SqsCallbackContext], None]

SqsResult = Union[SqsResultInteract, SqsResultSplit]
SqsResultPack = Union[SqsResultPackInteract, SqsResultPackSplit]

__all__ = [
    "Atom",
    "IterationMode",
    "LogLevel",
    "ParseError",
    "Prec",
    "SqsCallbackContext",
    "SqsCallbackContextDouble",
    "SqsCallbackContextFloat",
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
    "load_result_pack",
    "optimize",
    "parse_config",
    "random",
    "single",
    "split",
    "systematic",
]
