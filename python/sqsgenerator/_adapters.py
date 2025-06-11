import functools
import os.path
from typing import get_args

from typing_extensions import NamedTuple

from .core import Prec, Structure, StructureDouble, StructureFloat

try:
    from pymatgen.core import Structure as PymatgenStructure
except ImportError:
    HAVE_PYMATGEN = False
else:
    HAVE_PYMATGEN = True

try:
    from ase import Atoms
except ImportError:
    HAVE_ASE = False
else:
    HAVE_ASE = True


@functools.lru_cache(maxsize=2)
def _structure_type(prec: Prec) -> type[StructureFloat] | type[StructureDouble]:
    """
    Return the Structure class based on the precision.

    Args:
        prec (Prec): The precision type (single or double).

    Returns:
        type[StructureFloat] | type[StructureDouble]: The corresponding Structure class.

    Raises:
        ValueError: If the precision type is invalid.
    """
    match prec:
        case Prec.double:
            return StructureDouble
        case Prec.single:
            return StructureFloat
        case _:
            raise ValueError("Invalid prec")


if HAVE_PYMATGEN:
    from pymatgen.core.structure import FileFormats

    @functools.lru_cache(maxsize=1)
    def pymatgen_formats() -> tuple[str, ...]:
        """
        Return the available formats for pymatgen.

        Returns:
            tuple[str, ...]: A tuple of available pymatgen formats.
        """

        return tuple(fmt for fmt in get_args(FileFormats) if fmt)

    def to_pymatgen(structure: Structure) -> PymatgenStructure:
        """
        Convert a Structure to a pymatgen Structure.

        Args:
            structure (Structure): The input Structure object.

        Returns:
            PymatgenStructure: The converted pymatgen Structure object.
        """
        return PymatgenStructure(
            lattice=structure.lattice,
            species=[atom.symbol for atom in structure.atoms],
            coords=structure.frac_coords,
            coords_are_cartesian=False,
        )

    def from_pymatgen(ps: PymatgenStructure, prec: Prec = Prec.double) -> Structure:
        """
        Convert a pymatgen Structure to a Structure.

        Args:
            ps (PymatgenStructure): The pymatgen Structure object.
            prec (Prec, optional): The precision type. Defaults to Prec.double.

        Returns:
            Structure: The converted Structure object.
        """
        return _structure_type(prec)(
            ps.lattice.matrix,
            ps.frac_coords,
            [s.specie.Z for s in ps],
        )

    def write_pymatgen(
        structure: PymatgenStructure, filename: str, fmt: FileFormats
    ) -> None:
        """
        Write a pymatgen Structure to a file.

        Args:
            structure (PymatgenStructure): The pymatgen Structure object.
            filename (str): The file path to write to.
            fmt (FileFormats): The file format to use.
        """
        structure.to_file(filename, fmt)

    def read_pymatgen(filename: str, fmt: FileFormats) -> PymatgenStructure:
        """
        Read a pymatgen Structure from a file.

        Args:
            filename (str): The file path to read from.
            fmt (FileFormats): The file format to use.

        Returns:
            PymatgenStructure: The read pymatgen Structure object.
        """
        with open(filename) as f:
            return PymatgenStructure.from_str(f.read(), fmt)


if HAVE_ASE:

    class AseFileFormat(NamedTuple):
        extension: str
        capabilities: str
        code: str

    @functools.lru_cache(maxsize=1)
    def ase_formats() -> dict[str, AseFileFormat]:
        """
        Return the available formats for ASE.

        Returns:
            dict[str, AseFileFormat]: A dictionary of ASE formats with their details.
        """
        from ase.io.formats import all_formats

        fmts = {
            "abinit-in": "RW",
            "aims": "RW",
            "cfg": "RW",
            "cif": "RW+",
            "crystal": "RW",
            "cube": "RW",
            "dftb": "RW",
            "dlp4": "RW",
            "dmol-car": "RW",
            "dmol-incoor": "RW",
            "espresso-in": "RW",
            "gaussian-in": "RW",
            "gen": "RW",
            "gpumd": "RW",
            "gromacs": "RW",
            "gromos": "RW",
            "json": "RW+",
            "lammps-data": "RW",
            "magres": "RW",
            "mustem": "RW",
            "nwchem-in": "RW",
            "onetep-in": "RW",
            "prismatic": "RW",
            "res": "RW",
            "rmc6f": "RW",
            "struct": "RW",
            "sys": "RW",
            "traj": "RW+",
            "turbomole": "RW",
            "v-sim": "RW",
            "vasp": "RW",
            "xsd": "RW",
        }
        return {
            fmt: AseFileFormat(
                fmt,
                capabilities,
                all_formats[fmt].code,
            )
            for fmt, capabilities in fmts.items()
        }

    def to_ase(structure: Structure) -> Atoms:
        """
        Convert a Structure to an ASE Atoms object.

        Args:
            structure (Structure): The input Structure object.

        Returns:
            Atoms: The converted ASE Atoms object.
        """
        return Atoms(
            numbers=structure.species,
            scaled_positions=structure.frac_coords,
            cell=structure.lattice.matrix,
            pbc=True,
        )

    def from_ase(atom: Atoms, prec: Prec = Prec.double) -> Structure:
        """
        Convert an ASE Atoms object to a Structure.

        Args:
            atom (Atoms): The ASE Atoms object.
            prec (Prec, optional): The precision type. Defaults to Prec.double.

        Returns:
            Structure: The converted Structure object.
        """
        return _structure_type(prec)(
            atom.cell,
            atom.get_scaled_positions(),
            atom.numbers,
        )

    def read_ase(filename: str, fmt: str) -> Atoms:
        """
        Read an ASE Atoms object from a file.

        Args:
            filename (str): The file path to read from.
            fmt (str): The file format to use.

        Returns:
            Atoms: The read ASE Atoms object.

        Raises:
            ValueError: If the format is unsupported.
        """
        from ase.io import read

        if fmt not in ase_formats():
            raise ValueError(f"Unsupported ASE format: {fmt}")

        code = ase_formats[fmt].code
        if code.endswith("S"):
            out = read(filename, format=fmt)
        elif code.endswith("F"):
            with open(filename) as f:
                out = read(f, format=fmt)
        elif code.endswith("B"):
            with open(filename, "rb") as f:
                out = read(f, format=fmt)
        else:
            raise ValueError(f"Unsupported ASE format code: {code}")

        return out[0] if code.startswith("+") else out

    def write_ase(atoms: Atoms, filename: str, fmt: str) -> None:
        """
        Write an ASE Atoms object to a file.

        Args:
            atoms (Atoms): The ASE Atoms object.
            filename (str): The file path to write to.
            fmt (str): The file format to use.
        """
        from ase.io import write

        if fmt not in ase_formats():
            raise ValueError(f"Unsupported ASE format: {fmt}")

        code = ase_formats[fmt].code
        out = [atoms] if code.startswith("+") else atoms
        if code.endswith("S"):
            write(filename, out, format=fmt)
        elif code.endswith("F"):
            with open(filename, "w") as f:
                write(f, out, format=fmt)
        elif code.endswith("B"):
            with open(filename, "wb") as f:
                write(f, out, format=fmt)
        else:
            raise ValueError(f"Unsupported ASE format code: {code}")


def sqsgen_formats() -> tuple[str, ...]:
    return "vasp", "cif", "json"


def deduce_format(filename: str) -> str:
    """
    Deduce the file format from the file extension.

    Args:
        filename (str): The file path to deduce the format from.

    Returns:
        str: The deduced file format.
    """

    filename = os.path.basename(filename)
    if not filename:
        raise ValueError("filename cannot be empty.")

    parts = [part for part in filename.split(".") if part]
    if len(parts) < 2:
        raise ValueError("filename must have a valid extension.")
    elif len(parts) == 2:
        # there is no reader defined we use "sqsgen"
        if (fmt := parts[-1]) in sqsgen_formats():
            return "sqsgen", fmt
        else:
            raise ValueError(
                "sqsgen only supports one of this formats: "
                + ", ".join(sqsgen_formats())
            )
    else:
        match parts:
            case [*_, "sqsgen", fmt]:
                if fmt in sqsgen_formats():
                    return "sqsgen", fmt
                else:
                    raise ValueError(
                        "sqsgen only supports one of this formats: "
                        + ", ".join(sqsgen_formats())
                    )
            case [*_, "ase", fmt]:
                if not HAVE_ASE:
                    raise ImportError(
                        "ASE is not installed. Please install it to use this format."
                    )
                if fmt in ase_formats():
                    return "ase", fmt
                else:
                    raise ValueError(
                        "ase only supports one of this formats: "
                        + ", ".join(ase_formats().keys())
                    )
            case [*_, "pymatgen", fmt]:
                if not HAVE_PYMATGEN:
                    raise ImportError(
                        "Pymatgen is not installed. Please install it to use this format."
                    )
                if fmt in pymatgen_formats():
                    return "pymatgen", fmt
                else:
                    raise ValueError(
                        "pymatgen only supports one of this formats: "
                        + ", ".join(pymatgen_formats())
                    )


def write(
    structure: Structure,
    filename: str,
    fmt: str | None = None,
) -> None:
    """
    Write a structure to a file in the specified format.

    Args:
        structure (Structure): The structure to write.
        filename (str): The file path to write to.
        fmt (str): The file format to use.
    """

    backend, fmt = fmt or deduce_format(filename)
    match backend:
        case "ase":
            write_ase(to_ase(structure), filename, fmt)
        case "pymatgen":
            write_pymatgen(to_pymatgen(structure), filename, fmt)
        case "sqsgen":
            match fmt:
                case "cif":
                    pass
