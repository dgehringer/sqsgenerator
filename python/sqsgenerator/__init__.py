import warnings

from ._adapters import HAVE_ASE, HAVE_PYMATGEN, available_formats, read, write
from ._optimize import optimize, parse_config
from .core import (
    IterationMode,
    Prec,
    StructureFormat,
    SublatticeMode,
    __version__,
    load_result_pack,
)

__all__ = [
    "HAVE_ASE",
    "HAVE_PYMATGEN",
    "IterationMode",
    "Prec",
    "StructureFormat",
    "SublatticeMode",
    "available_formats",
    "load_result_pack",
    "optimize",
    "parse_config",
    "read",
    "write",
]

warnings.warn(
    "The current version of sqsgenerator is a pre-release. We have introduced breaking changes in versions >= 0.4.*. We are still working on updating the documentation. The last stable version is 0.3 and can be obtained via conda",
    UserWarning,
    stacklevel=2,
)


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
