"""
Provides utilities for exporting structure objects with ``ase`` and ``pymatgen``.
Tools for reading/writings settings files
"""

import io
import tarfile
import zipfile
import functools
import numpy as np
import typing as T
from frozendict import frozendict
from sqsgenerator.fallback.attrdict import AttrDict
from sqsgenerator.core import Structure, IterationMode
from sqsgenerator.compat import FeatureNotAvailableException
from operator import attrgetter as attr, methodcaller as method
from sqsgenerator.compat import require, have_feature, get_module, Feature
from sqsgenerator.adapters import from_ase_atoms, from_pymatgen_structure, to_pymatgen_structure, to_ase_atoms


F = Feature
known_adapters = (F.ase, F.pymatgen)

compression_to_file_extension = frozendict(zip='zip', bz2='tar.bz2', gz='tar.gz', xz='tar.xz')

output_formats = {
    F.pymatgen: {'cif', 'mcif', 'poscar', 'cssr', 'json', 'xsf', 'prismatic', 'yaml'}
}


def identity(x: T.Any) -> T.Any:
    return x


if have_feature(F.ase):
    from ase.io.formats import all_formats, get_ioformat

    # the underlying ase write function must be capable to cope with file handles
    # therefore we use get_ioformat to get meta-information about the formats, and sort out the unsuitable
    output_formats[F.ase] = {f for f in all_formats.keys() if get_ioformat(f).acceptsfd}


def prepare_handle(fp: T.IO[bytes], feature: Feature, format: str) -> T.Union[T.IO[bytes], T.IO[str]]:
    """
    Sanitizes a file-like by wrapping it in a TextIO if needed

    :param fp: the file-like to sanitize
    :type fp: IO[bytes]
    :param feature: the file writer module "ase" or "pymatgen"
    :type feature: Feature
    :param format: the file format e.g "cif"
    :type format: str
    :return: the file handle
    :rtype: IO[bytes] or IO[str]
    """
    assert format in output_formats[feature]

    if feature == F.pymatgen:
        return io.TextIOWrapper(fp)
    elif feature == F.ase:
        return fp if get_ioformat(format).isbinary else io.TextIOWrapper(fp)


def default_adapter():
    """
    Gets the default writer/reader module to read files

    :return: the default
    :rtype: Feature or None
    """
    for a in known_adapters:
        if have_feature(a): return a
    return None


def supported_formats(feature=None):
    """
    For a given feature ("ase" or "pymatgen") the supported output file formats computed. If None is passed the values
    from ``default_adapter()`` will be used

    :param feature: the writer/reader feature (default is None)
    :type feature: Feature
    :return: the supported output formats
    :rtype: set
    """
    return output_formats.get(feature or default_adapter(), {})


class NonCloseableBytesIO(io.BytesIO):
    """
    Class as workaround. Some ase.io writer functions close the buffer so that we cannot capture their output
    I/O operation on closed buffer Exception. Therefore BytesIO with dummy
    """

    def close(self) -> None:
        pass

    def really_close(self):
        super(NonCloseableBytesIO, self).close()


def capture(f):
    """
    Decorator which captures the written bytes of a "write" function an. It passed the buffer as the first argument
    to the wrapped function

    :param f: a callable returning the written bytes
    :type f: callable
    :return: wrapped function
    """

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
def dumps(o: dict, output_format: str = 'yaml') -> bytes:
    """
    Dumps a dict-like object into a byte string, using a backend specified by {output_format}

    :param o: a dict-like object which is serializable by the module specified by {output_format}
    :param output_format: backend used to store (json, pickle, yaml) the object (default is ``"yaml"``
    :type output_format: str
    :return: the dumped content
    :rtype: bytes
    """
    f = F(output_format)
    if not have_feature(f):
        raise FeatureNotAvailableException(f'The package "{format}" is not installed, '
                                           'consider to install it with')

    # for yaml format we create a simple wrapper which captures the output
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
def read_settings_file(path: str, format: str = 'yaml') -> AttrDict:
    """
    Reads a file expecting {format} as the file type. This method does not process the input paramters, but rather
    just reads, them from. To obtain default values for all parameters use :py:func:`process_settings`

    :param path: the file path
    :type path: str
    :param format: the input file-type. Possible formats are *yaml*, *json* and *pickle* (default is ``'yaml'``)
    :type format: str
    :return: the parsed settings
    :rtype: AttrDict
    """
    f = F(format)
    readers = {
        F.json: 'loads',
        F.pickle: 'loads',
        F.yaml: 'safe_load'
    }
    if not have_feature(f):
        raise FeatureNotAvailableException(f'The package "{format}" is not installed, '
                                           'consider to install it with')
    reader = getattr(get_module(f), readers[f])
    try:
        mode = 'r' if f != F.pickle else 'rb'
        with open(path, mode) as settings_file:
            content = settings_file.read()
    except (FileNotFoundError, UnicodeDecodeError):
        raise
    try:
        data = AttrDict(reader(content))
    except Exception as e:
        raise IOError(f'While reading the file "{path}", a "{type(e).__name__}" occurred. '
                      f'Maybe the file has the wrong format. '
                      f'I was expecting a "{format}"-file. '
                      f'You can specify a different input-file format using the "--format" option')
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
def write_structure_file(fp: T.IO[bytes], structure: Structure, format: str, writer: Feature = default_adapter(),
                         **kwargs) -> T.NoReturn:
    """
    Write a ``sqsgenerator.core.Structure`` object into a file, with file format {format} using {writer} as backend

    :param fp: file object
    :type fp: IO[bytes]
    :param structure: the structure to save
    :type structure: Structure
    :param format: file type used. Must be supported by {writer}
    :type format: str
    :param writer: the writer backend (default is ``default_adapter()``)
    :type writer: Feature
    :param kwargs: keyword arguments passed to the backends
    """
    writer_funcs = {
        F.ase: write_structure_file_with_ase,
        F.pymatgen: write_structure_file_with_pymatgen
    }
    fh = prepare_handle(fp, writer, format)
    writer_funcs[writer](fh, structure, format, **kwargs)
    fh.flush()


@require(F.ase, F.pymatgen, condition=any)
def read_structure_from_file(settings: AttrDict) -> Structure:
    """
    Read a structure object from

    :param settings: the settings dictionary
    :type settings: AttrDict
    :return: the Structure object
    :rtype: Structure
    """
    reader = settings.structure.get('reader', 'ase')
    available_readers = set(map(attr('value'), known_adapters))
    if reader not in available_readers:
        raise FeatureNotAvailableException(f'Unknown reader specification "{reader}.  '
                                           f'Available readers are {known_adapters}')
    reader_kwargs = settings.structure.get('args', {})
    reader_funcs = dict(ase=read_structure_file_with_ase, pymatgen=read_structure_file_with_pymatgen)
    return reader_funcs[reader](settings.structure.file, **reader_kwargs)


# Helper to capture the backends output into bytes
dumps_structure = capture(write_structure_file)


def to_dict(settings: dict) -> T.Dict[str, T.Any]:
    """
    Utility method to recursively turn a general dictionary into an JSON/YAML serializable dictionary.
    If a non-trivial object is encountered the function searches for a ``to_dict()`` function. If it has no method
    available to serialize the object a ``TypeError`` is raised. **Attention:** the function serializes ``np.ndarray``
    by calling ``tolist()``. This is not a good idea but fits the needs in this project

    :param settings: a generic dictionary object
    :type settings: dict
    :return: a serializable dict
    :rtype: dict
    """

    identity = lambda _: _  # although bad practice this is readable =)
    converters = {
        int: identity,
        float: identity,
        str: identity,
        bool: identity,
        IterationMode: str,
        np.ndarray: method('tolist')
    }

    def _generic_to_dict(d):
        td = type(d)
        if isinstance(d, (tuple, list, set)):
            return td(map(_generic_to_dict, d))
        elif isinstance(d, dict):
            return dict({k: _generic_to_dict(v) for k, v in d.items()})
        elif td in converters:
            return converters[td](d)
        elif hasattr(d, 'to_dict'):
            return d.to_dict()
        else:
            raise TypeError(f'No converter specified for type "{td}"')

    return _generic_to_dict(settings)


def export_structures(structures: T.Dict[T.Any, T.Any], format: str = 'cif', output_file: str = 'sqs.result',
                      writer: T.Union[Feature,str] = 'ase', compress: T.Optional[str] = None,
                      functor: T.Callable[[T.Any], str] = identity) -> T.NoReturn:
    """
    Writes structures into files. The filename is specified by the keys of {structure} argument. The structures stored
    in the values will be written using the {writer} backend in {format}. If compress is specified the structures will
    be dumped into an archive with name {output_file}. The file-extension is chosen automatically.

    :param structures: a mapping of filenames and :py:class:`Structures`
    :type structures: dict[Any, Structure]
    :param format: output file format (default is ``'cif'``)
    :param output_file: the prefix of the output archive name. File extension is chosen automatically.
        If {compress} is ``None`` this option is ignored (default is ``'sqs.result'``)
    :type output_file: str
    :param writer: the writer backend (default is ``'ase'``)
    :type writer: str
    :param compress: compression algorithm (``'zip'``, ``'gz'``, ``'bz2'`` or ``'xz'``) used to store the structure
        files. If ``None`` the structures are written to plain files (default is ``None``)
    :type compress: str or None
    :param functor: a callable which maps the values of {structures} on a :py:class:`Structure` (default is ``identity = lambda x: x``)
    :type functor: Callable[[Any], Structure]

    """

    writer = Feature(writer) if isinstance(writer, str) else writer

    output_prefix = output_file
    if compress:
        # select the proper file-mode as well as file-name and opening method
        output_archive_file_mode = f'x:{compress}' if compress != 'zip' else 'x'
        output_archive_name = f'{output_prefix}.{compression_to_file_extension.get(compress)}'
        open_ = tarfile.open if compress != 'zip' else zipfile.ZipFile
        archive_handle = open_(output_archive_name, output_archive_file_mode)
    else:
        archive_handle = None

    # helper method, dealing with the compression algorithms
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

    structures = {k: functor(v) for k, v in structures.items()}
    for rank, structure in structures.items():
        filename = f'{rank}.{format}'
        data = dumps_structure(structure, format, writer=writer)  # capture the output from the {writer} backend
        write_structure_dump(data, filename)

    if compress:
        # If we dumped everything into an archive, we have to close the it at the end
        archive_handle.close()
