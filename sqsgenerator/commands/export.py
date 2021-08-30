import os
import click
from attrdict import AttrDict
from sqsgenerator.compat import Feature as F
from sqsgenerator.main import extract_structures
from sqsgenerator.settings.readers import read_structure
from sqsgenerator.commands.common import click_settings_file, error
from sqsgenerator.io import export_structures as export_structures_io, supported_formats ,compression_to_file_extension


def export_structures(settings, result_document, output_file=None, format='cif', writer='ase', compress=None):

    output_prefix = output_file \
        if output_file is not None \
        else (os.path.splitext(settings.file_name)[0] if 'file_name' in settings else 'sqs')

    writer = F(writer)
    if format not in supported_formats(writer):
        error(f'{writer.value} does not support the format "{format}". '
              f'Supported formats are {supported_formats(writer)}', prefix='FeatureError')

    structures = extract_structures(result_document)
    export_structures_io(structures, format=format, output_file=output_prefix, writer=writer, compress=compress)


@click.command('export')
@click.option('--format', '-f', type=click.STRING, default='cif')
@click.option('--writer', '-w', type=click.Choice(['ase', 'pymatgen']), default='ase')
@click.option('--compress', '-c', type=click.Choice(list(compression_to_file_extension)))
@click.option('--output', '-o', 'output_file', type=click.Path(dir_okay=False, allow_dash=True))
@click_settings_file(process=None, default_name='sqs.result.yaml')
def export(settings, format='cif', writer='ase', compress=None, output_file='sqs.result'):

    result_document = AttrDict(settings)
    needed_keys = {'structure', 'configurations'}
    if not all(map(lambda k: k in result_document, needed_keys)):
        error(f'Result document must at least contain the following keys: {needed_keys}', prefix='KeyError')
    # read the structure from from the settings document

    result_document['structure'] = read_structure(result_document)
    export_structures(settings, result_document, format=format, writer=writer, compress=compress)