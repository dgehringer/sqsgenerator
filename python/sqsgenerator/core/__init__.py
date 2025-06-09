import os
import sys

if sys.platform == "win32":
    # On Windows, we need to add the directory of the current file to the DLL search path for Python 3.12+
    os.add_dll_directory(os.path.dirname(__file__))

from ._core import (
    IterationMode,
    LogLevel,
    ParseError,
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

SqsConfiguration = SqsConfigurationFloat | SqsConfigurationDouble
Structure = StructureFloat | StructureDouble
SqsResultSplit = SqsResultSplitFloat | SqsResultSplitDouble
SqsResultInteract = SqsResultInteractFloat | SqsResultInteractDouble
SqsResultPackSplit = SqsResultPackSplitFloat | SqsResultPackSplitDouble
SqsResultPackInteract = SqsResultPackInteractFloat | SqsResultPackInteractDouble

SqsResult = SqsResultInteract | SqsResultSplit
SqsResultPack = SqsResultPackInteract | SqsResultPackSplit

__all__ = [
    "IterationMode",
    "LogLevel",
    "ParseError",
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
    "load_result_pack",
    "optimize",
    "parse_config",
    "random",
    "single",
    "split",
    "systematic",
]
