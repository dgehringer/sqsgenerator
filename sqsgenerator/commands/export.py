"""
Utility command to export the internal dict-like results into structure files
"""

import os
import click
from attrdict import AttrDict
from sqsgenerator.public import extract_structures
from sqsgenerator.settings.readers import read_structure
from sqsgenerator.compat import Feature as F, have_feature
from sqsgenerator.commands.common import click_settings_file, error
from sqsgenerator.commands.help import command_help as c_help, parameter_help as help
from sqsgenerator.io import export_structures as export_structures_io, supported_formats, compression_to_file_extension


def export_structures(settings, result_document, output_file=None, format='cif', writer='ase', compress=None):
    """
    Helper function for `sqsgenerator.io.export_structures`. Checks if the requested backend is available and
    the output format is supported by the backend
    """

    # generate a output file name
    #  - in case it was called from the CLI @click_settings_file injected a 'file_name' key into settings
    #  - we use this key if {output_file} was not explicitly specified
    #  - if neither is the case we fall back to "sqs"
    output_prefix = output_file \
        if output_file is not None \
        else (os.path.splitext(settings.file_name)[0] if 'file_name' in settings else 'sqs')

    writer = F(writer)
    # check backend is available
    if not have_feature(writer):
        error(f'I cannot use "{writer.value}". It seems it is not installed', prefix='FeatureError')

    # check if backend supports {format}
    if format not in supported_formats(writer):
        error(f'{writer.value} does not support the format "{format}". '
              f'Supported formats are {supported_formats(writer)}', prefix='FeatureError')

    structures = extract_structures(result_document)
    export_structures_io(structures, format=format, output_file=output_prefix, writer=writer, compress=compress)


@click.command('export', help=c_help.export)
@click.option('--format', '-f', type=click.STRING, default='cif', help=help.format)
@click.option('--writer', '-w', type=click.Choice(['ase', 'pymatgen']), default='ase', help=help.writer)
@click.option('--compress', '-c', type=click.Choice(list(compression_to_file_extension)), help=help.compress)
@click.option('--output', '-o', 'output_file', type=click.Path(dir_okay=False, allow_dash=True), help=help.output_file)
@click_settings_file(process=None, default_name='sqs.result.yaml')
def export(settings, format='cif', writer='ase', compress=None, output_file='sqs.result'):

    result_document = AttrDict(settings)
    needed_keys = {'structure', 'configurations'}

    # check if data neeeded for export is there at all
    if not all(map(lambda k: k in result_document, needed_keys)):
        error(f'Result document must at least contain the following keys: {needed_keys}', prefix='KeyError')

    result_document['structure'] = read_structure(result_document)
    export_structures(settings, result_document, format=format, writer=writer, compress=compress,
                      output_file=output_file)
