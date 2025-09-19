import warnings

from ._adapters import HAVE_ASE, HAVE_PYMATGEN, available_formats, read, write
from ._optimize import optimize, parse_config
from .core import (
    Atom,
    IterationMode,
    Prec,
    SqsResult,
    SqsResultPack,
    StructureFormat,
    SublatticeMode,
    __version__,
    load_result_pack,
)

__all__ = [
    "HAVE_ASE",
    "HAVE_PYMATGEN",
    "Atom",
    "IterationMode",
    "Prec",
    "SqsResult",
    "SqsResultPack",
    "StructureFormat",
    "SublatticeMode",
    "__version__",
    "available_formats",
    "load_result_pack",
    "optimize",
    "parse_config",
    "read",
    "write",
]


if HAVE_PYMATGEN:
    from ._adapters import (
        from_pymatgen,
        pymatgen_formats,
        read_pymatgen,
        to_pymatgen,
        write_pymatgen,
    )

    __all__ += [
        "from_pymatgen",
        "pymatgen_formats",
        "read_pymatgen",
        "to_pymatgen",
        "write_pymatgen",
    ]

if HAVE_ASE:
    from ._adapters import (
        ase_formats,
        from_ase,
        read_ase,
        to_ase,
        write_ase,
    )

    __all__ += ["ase_formats", "from_ase", "read_ase", "to_ase", "write_ase"]
