
import os
import io
import tarfile
import zipfile
import attrdict
import functools
import frozendict
import typing as T
import numpy as np
from operator import attrgetter as attr, methodcaller as method
from sqsgenerator.core import Structure, IterationMode
from sqsgenerator.compat import FeatureNotAvailableException
from sqsgenerator.compat import require, have_feature, get_module, Feature as F
from sqsgenerator.adapters import from_ase_atoms, from_pymatgen_structure, to_pymatgen_structure, to_ase_atoms

known_adapters = (F.ase, F.pymatgen)

compression_to_file_extension = frozendict.frozendict(zip='zip', bz2='tar.bz2', gz='tar.gz', xz='tar.xz')

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


@require(F.json, F.yaml, F.pickle, condition=any)
def dumps(o: dict, output_format='yaml'):
    f = F(output_format)
    if not have_feature(f): raise FeatureNotAvailableException(f'The package "{format}" is not installed, consider to install it with')

    def safe_dumps(d, **kwargs):
        buf = io.StringIO()
        get_module(F.yaml).safe_dump(d, buf, **kwargs)
        return buf.getvalue()

    dumpers = {
        F.json: lambda d: get_module(F.json).dumps(d, indent=4),
        F.pickle: lambda d: get_module(F.pickle).dumps(d),
        F.yaml: lambda d: safe_dumps(d, default_flow_style=None)
    }
    content = dumpers[f](o)
    return content if f == F.pickle else content.encode()


@require(F.yaml, F.json, condition=any)
def read_settings_file(path, format='yaml') -> T.Optional[attrdict.AttrDict]:
    f = F(format)
    readers = {
        F.json: 'loads',
        F.pickle: 'loads',
        F.yaml: 'safe_load'
    }
    if not have_feature(f):
        raise FeatureNotAvailableException(f'The package "{format}" is not installed, consider to install it with')
    reader = getattr(get_module(f), readers[f])
    try:
        with open(path, 'r' if f != F.pickle else 'rb') as settings_file:
            content = settings_file.read()
    except FileNotFoundError:
        raise
    try:
        data = attrdict.AttrDict(reader(content))
    except Exception as e:
        prefix = type(e).__name__
        raise IOError(f'While reading the file "{path}", a "{prefix}" occurred. Maybe the file has the wrong format. I was expecting a "{format}"-file. You can specify a different input-file format using the "--format" option')
    return data


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


@require(F.ase, F.pymatgen, condition=any)
def read_structure_from_file(settings: attrdict.AttrDict):
    reader = settings.structure.get('reader', 'ase')
    available_readers = set(map(attr('value'), known_adapters))
    if reader not in available_readers: raise FeatureNotAvailableException(f'Unknown reader specification "{reader}". Available readers are {known_adapters}')
    reader_kwargs = settings.structure.get('args', {})
    reader_funcs = dict(ase=read_structure_file_with_ase, pymatgen=read_structure_file_with_pymatgen)
    return reader_funcs[reader](settings.structure.file, **reader_kwargs)


dumps_structure = capture(write_structure_file)


def to_dict(settings: dict) -> T.Dict[str, T.Any]:
    identity = lambda x: x
    converters = {
        int: identity,
        float: identity,
        str: identity,
        bool: identity,
        Structure: method('to_dict'),
        IterationMode: str,
        np.ndarray: method('tolist')
    }

    def _generic_to_dict(d):
        td = type(d)
        if isinstance(d, (tuple, list, set)):
            return td(map(_generic_to_dict, d))
        elif isinstance(d, dict):
            return dict( { k: _generic_to_dict(v) for k, v in d.items() } )
        elif td in converters:
            return converters[td](d)
        elif hasattr(d, 'to_dict'):
            return d.to_dict()
        else: raise TypeError(f'No converter specified for type "{td}"')

    out = _generic_to_dict(settings)
    return out


def export_structures(structures: T.Dict[int, Structure], format='cif', output_file='sqs.result', writer='ase', compress=None):
    output_prefix = output_file
    if compress:
        output_archive_file_mode = f'x:{compress}' if compress != 'zip' else 'x'
        output_archive_name = f'{output_prefix}.{compression_to_file_extension.get(compress)}'
        open_ = tarfile.open if compress != 'zip' else zipfile.ZipFile
        archive_handle = open_(output_archive_name, output_archive_file_mode)
    else:
        archive_handle = None

    def write_structure_dump(data: bytes, filename: str):
        if not compress:
            with open(filename, 'wb') as fh:
                fh.write(data)
        else:
            if compress == 'zip':
                assert isinstance(archive_handle, zipfile.ZipFile)
                archive_handle.writestr(filename, data)
            else:
                assert isinstance(archive_handle, tarfile.TarFile)
                with io.BytesIO(data) as buf:
                    tar_info = tarfile.TarInfo(name=filename)
                    tar_info.size = len(data)
                    archive_handle.addfile(tar_info, buf)

    for rank, structure in structures.items():
        filename = f'{rank}.{format}'
        data = dumps_structure(structure, format, writer=writer)
        write_structure_dump(data, filename)

    if compress: archive_handle.close()
