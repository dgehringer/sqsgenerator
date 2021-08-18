import io
import os
import time
import click
import shutil
import tarfile
import zipfile
import attrdict
import functools
import frozendict
import numpy as np
import typing as T
from sqsgenerator.compat import Feature as F
from sqsgenerator.settings import construct_settings, settings_to_dict, supported_formats, BadSettings
from sqsgenerator.settings.adapters import write_structure_file, dumps_structure
from sqsgenerator.commands.common import click_settings_file, dumps, error
from sqsgenerator.core import pair_sqs_iteration, set_core_log_level, log_levels, symbols_from_z, SQSResult, Structure
from operator import attrgetter as attr

TimingDictionary = T.Dict[int, T.List[float]]
Settings = attrdict.AttrDict

compression_to_file_extension = frozendict.frozendict(zip='zip', bz2='tar.bz2', gz='tar.gz', xz='tar.xz')


def make_result_document(sqs_results: T.Iterable[SQSResult], settings: Settings, timings : T.Optional[TimingDictionary]=None, fields: T.Tuple[str]=('configuration',)) -> Settings:
    allowed_fields = {
        'configuration': lambda result: symbols_from_z(result.configuration),
        'objective': attr('objective'),
        'parameters': lambda result: result.parameters(settings.target_objective.shape)
    }

    def make_sqs_result_document(result):
        if len(fields) == 1:
            key = next(iter(fields))
            return allowed_fields[key](result)
        else:
            return {f: allowed_fields[f](result) for f in fields}

    have_sublattice = 'sublattice' in settings
    result_document = {
        'structure': settings.sublattice.structure if have_sublattice else settings.structure,
        'configurations': {r.rank: make_sqs_result_document(r) for r in sqs_results}
    }
    if have_sublattice:
        result_document['which'] = settings.sublattice.which
    if timings is not None: result_document['timings'] = timings
    return Settings(result_document)


def expand_results(dense: Settings):
    have_sublattice = 'which' in dense
    s = dense.structure
    new_structure = functools.partial(Structure, s.lattice, s.frac_coords)

    def new_species(conf):
        if have_sublattice:
            species = s.symbols
            mask = np.array(dense.which, dtype=int)
            species[mask] = conf
            return species.tolist()
        else: return conf

    return {rank: new_structure(new_species(conf)) for rank, conf in dense.configurations.items()}



@click.command('run')
@click.option('--log-level', type=click.Choice(list(log_levels.keys())), default='warning')
@click.option('--export', '-e', is_flag=True)
@click.option('--format', '-f', type=click.STRING, default='cif')
@click.option('--writer', '-w', type=click.Choice(['ase', 'pymatgen']), default='ase')
@click.option('--compress', '-c', type=click.Choice(list(compression_to_file_extension)))
@click_settings_file('all')
def run(settings, log_level, export, format, writer, compress):
    iteration_settings = construct_settings(settings, False)
    set_core_log_level(log_levels.get(log_level))
    sqs_results, timings = pair_sqs_iteration(iteration_settings)

    result_document = make_result_document(sqs_results, settings)
    output_prefix = os.path.splitext(settings.file_name)[0] if 'file_name' in settings else 'sqs'

    if export:
        writer = F(writer)
        if format not in supported_formats(writer): error(f'{writer.value} does not support the format "{format}". Supported formats are {supported_formats(writer)}')
        structures = expand_results(result_document)

        if compress:
            output_archive_file_mode = f'x:{compress}' if compress != 'zip' else 'x'
            output_archive_name = f'{output_prefix}.{compression_to_file_extension.get(compress)}'
            open_ = tarfile.open if compress != 'zip' else zipfile.ZipFile
            archive_handle = open_(output_archive_name, output_archive_file_mode)
        else: archive_handle = None

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

    ##print(dumps(result_document).decode())
