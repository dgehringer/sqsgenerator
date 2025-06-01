import functools

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
    """Return the Structure class based on the precision."""
    match prec:
        case Prec.double:
            return StructureDouble
        case Prec.single:
            return StructureFloat
        case _:
            raise ValueError("Invalid prec")


if HAVE_PYMATGEN:

    def to_pymatgen(structure: Structure) -> PymatgenStructure:
        """Convert a Structure to a pymatgen Structure."""
        return PymatgenStructure(
            lattice=structure.lattice,
            species=[atom.symbol for atom in structure.atoms],
            coords=structure.frac_coords,
            coords_are_cartesian=False,
        )

    def from_pymatgen(ps: PymatgenStructure, prec: Prec = Prec.double) -> Structure:
        """Convert a pymatgen Structure to a Structure."""
        return _structure_type(prec)(
            ps.lattice.matrix,
            ps.frac_coords,
            [s.specie.Z for s in ps],
        )


if HAVE_ASE:

    def to_ase(structure: Structure) -> Atoms:
        """Convert a Structure to an ASE Atoms object."""
        return Atoms(
            numbers=structure.species,
            scaled_positions=structure.frac_coords,
            cell=structure.lattice.matrix,
            pbc=True,
        )

    def from_ase(atom: Atoms, prec: Prec = Prec.double) -> Structure:
        """Convert an ASE Atoms object to a Structure."""
        return _structure_type(prec)(
            atom.cell,
            atom.get_scaled_positions(),
            atom.numbers,
        )
