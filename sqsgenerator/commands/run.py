

import os
import click
from sqsgenerator.commands.export import export_structures
from sqsgenerator.commands.common import click_settings_file
from sqsgenerator.io import to_dict, dumps, compression_to_file_extension
from sqsgenerator.main import log_levels, pair_sqs_iteration, expand_sqs_results


@click.command('iteration')
@click.option('--minimal/--no-minimal', '-m/-nm', default=True)
@click.option('--similar/--no-similar', '-s/-ns', default=False)
@click.option('--log-level', type=click.Choice(list(log_levels.keys())), default='warning')
@click.option('--dump/--no-dump', '-d/-nd', default=True)
@click.option('--export', '-e', 'do_export', is_flag=True)
@click.option('--dump-params', '-dp', is_flag=True)
@click.option('--format', '-f', type=click.STRING, default='cif')
@click.option('--writer', '-w', type=click.Choice(['ase', 'pymatgen']), default='ase')
@click.option('--output', '-o', 'output_file', type=click.Path(dir_okay=False, allow_dash=True))
@click.option('--dump-format', '-df', 'dump_format', type=click.Choice(['yaml', 'json', 'pickle']), default='yaml')
@click.option('--dump-include', '-di', 'dump_include', type=click.Choice(['parameters', 'timings', 'objective']), multiple=True)
@click.option('--compress', '-c', type=click.Choice(list(compression_to_file_extension)))
@click_settings_file('all')
def iteration(settings, minimal, similar, log_level, dump, dump_params, do_export, format, writer, output_file, dump_format, dump_include, compress):
    sqs_results, timings = pair_sqs_iteration(settings, minimal=minimal, similar=similar, log_level=log_level)

    final_document = expand_sqs_results(settings, sqs_results, timings, include=dump_include, inplace=dump_params)
    output_prefix = output_file \
        if output_file is not None \
        else (os.path.splitext(settings.file_name)[0] if 'file_name' in settings else 'sqs')

    if dump:
        output_file_name = f'{output_prefix}.result.{dump_format}'
        with open(output_file_name, 'wb') as handle:
            handle.write(
                dumps(
                    to_dict(final_document), output_format=dump_format
                ))

    if do_export:
        export_structures(settings, final_document, format=format, writer=writer, output_file=output_prefix, compress=compress)
    return final_document


@click.group('run')
def run():
    pass


run.add_command(iteration, 'iteration')
