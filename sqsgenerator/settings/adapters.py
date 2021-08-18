import io
import functools
import attrdict
import numpy as np
from .functional import try_
from .exceptions import BadSettings
from operator import attrgetter as attr
from sqsgenerator.core import Structure
from sqsgenerator.compat import Feature as F, have_feature, require
import typing as T


unique_species = attr('unique_species')
num_species = attr('num_unique_species')


def transpose(iterable: T.Iterable): return zip(*iterable)


class NonCloseableBytesIO(io.BytesIO):
    """
    Class as workaround. Some ase.io writer function close the buffer so that we cannot capture their output
    I/O operation on closed buffer Exception. Therefore BytesIO with dummy
    """

    def close(self) -> None:
        pass

    def really_close(self):
        super(NonCloseableBytesIO, self).close()


def capture(f):

    @functools.wraps(f)
    def _capture(*args, **kwargs):
        fp = NonCloseableBytesIO()
        f(fp, *args, **kwargs)
        fp.seek(0)
        r = fp.getvalue()
        fp.really_close()
        return r

    return _capture


known_adapters = (F.ase, F.pymatgen)


output_formats = {
    F.pymatgen: {'cif', 'mcif', 'poscar', 'cssr', 'json', 'xsf', 'prismatic', 'yaml'}
}

if have_feature(F.ase):
    from ase.io.formats import all_formats, get_ioformat
    # the underlying ase write function must be capable to cope with file handles
    # therefore we use get_ioformat to get meta-information about the formats, and sort out the unsuitable
    output_formats[F.ase] = {f for f in all_formats.keys() if get_ioformat(f).acceptsfd}


def prepare_handle(fp, feature, format, compression=None):
    assert format in output_formats[feature]

    if feature == F.pymatgen:
        return io.TextIOWrapper(fp)
    elif feature == F.ase:
        return fp if get_ioformat(format).isbinary else io.TextIOWrapper(fp)


def default_adapter():
    for a in known_adapters:
        if have_feature(a): return a
    return None


def supported_formats(feature=None):
    return output_formats.get(feature or default_adapter())


@require(F.ase, F.pymatgen, condition=any)
def read_structure_from_file(settings: attrdict.AttrDict):
    reader = settings.structure.get('reader', 'ase')
    available_readers = set(map(attr('value'), known_adapters))
    if reader not in available_readers: raise BadSettings(f'Unknown reader specification "{reader}". Available readers are {known_adapters}')
    reader_kwargs = settings.structure.get('args', {})
    reader_funcs = dict(ase=read_structure_file_with_ase, pymatgen=read_structure_file_with_pymatgen)
    return try_(reader_funcs[reader], settings.structure.file, **reader_kwargs, msg=f'read structure file "{settings.structure.file}"')


@require(F.pymatgen)
def to_pymatgen_structure(structure: Structure):
    from pymatgen.core import Structure as PymatgenStructure, Lattice as PymatgenLattice
    symbols = list(map(attr('symbol'), structure.species))
    lattice = PymatgenLattice(structure.lattice)
    return PymatgenStructure(lattice, symbols, structure.frac_coords, coords_are_cartesian=False, validate_proximity=True)


@require(F.ase)
def to_ase_atoms(structure: Structure):
    from ase import Atoms
    numbers = list(map(attr('Z'), structure.species))
    return Atoms(cell=structure.lattice, scaled_positions=structure.frac_coords, numbers=numbers, pbc=structure.pbc)


@require(F.pymatgen)
def from_pymatgen_structure(structure) -> Structure:
    lattice = structure.lattice.matrix
    data = map(lambda site: (site.species_string, site.frac_coords), structure)
    species, frac_coords = transpose(data)
    return Structure(np.array(lattice), np.array(frac_coords), species, (True, True, True))


@require(F.ase)
def from_ase_atoms(atoms) -> Structure:
    lattice = np.array(atoms.cell)
    frac_coords = np.array(atoms.get_scaled_positions())
    if tuple(atoms.pbc) != (True, True, True): raise RuntimeWarning("At present I can only handle fully periodic structure. I'will overwrite ase.Atoms.pbc setting to (True, True, True)")
    return Structure(lattice, frac_coords, list(atoms.symbols), (True, True, True))


@require(F.ase)
def read_structure_file_with_ase(fn, **kwargs) -> Structure:
    import ase.io
    return from_ase_atoms(ase.io.read(fn, **kwargs))


@require(F.pymatgen)
def read_structure_file_with_pymatgen(fn, **kwargs) -> Structure:
    import pymatgen.core
    return from_pymatgen_structure(pymatgen.core.Structure.from_file(fn, **kwargs))


@require(F.ase)
def write_structure_file_with_ase(fp, structure: Structure, format, sort=True, **kwargs):
    import ase.io
    ase.io.write(fp, (to_ase_atoms(structure.sorted() if sort else structure),), format=format, **kwargs)


@require(F.pymatgen)
def write_structure_file_with_pymatgen(fp, structure: Structure, format, sort=True, **kwargs):
    fp.write(to_pymatgen_structure(structure.sorted() if sort else structure).to(format, **kwargs))


@require(F.ase, F.pymatgen, condition=any)
def write_structure_file(fp, structure: Structure, format, writer=default_adapter(), **kwargs):
    writer_funcs = {
        F.ase :write_structure_file_with_ase,
        F.pymatgen: write_structure_file_with_pymatgen
    }
    fh = prepare_handle(fp, writer, format)
    writer_funcs[writer](fh, structure, format, **kwargs)
    fh.flush()


dumps_structure = capture(write_structure_file)