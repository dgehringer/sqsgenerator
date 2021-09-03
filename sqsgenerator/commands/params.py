"""
Command to check the actual parameters which are going to be used in a SQS iteration
"""

import sys
import click
from sqsgenerator.io import dumps, to_dict
from sqsgenerator.commands.common import pretty_print, click_settings_file
from sqsgenerator.commands.help import command_help as c_help, parameter_help as help
from sqsgenerator.settings import process_settings, parameter_list, construct_settings


@click.command('show', help=c_help.params.show)
@click.option('--param', '-p', type=click.Choice(parameter_list()), default=None, multiple=True, help=help.param)
@click.option('--output-format', '-of', type=click.Choice(['yaml', 'json', 'pickle', 'native']), default='native',
              help=help.output_format)
@click.option('--inplace', is_flag=True, help=help.inplace)
@click_settings_file(process=None)
def show(settings, param=None, output_format='yaml', inplace=False):
    parameters_selected = bool(param)
    parameters_to_process = set(param or parameter_list())
    processed = process_settings(settings, params=parameters_to_process)
    parameters_to_output = parameters_to_process.union({} if parameters_selected else {'sublattice'})
    filtered = {k: v for k, v in to_dict(processed).items() if k in parameters_to_output}

    if inplace:
        output_format = settings.input_format
        output_file = open(settings.file_name, 'wb')
    else:
        output_file = sys.stdout.buffer

    if output_format == 'native':
        pretty_print(filtered)
    else:
        output_file.write(dumps(filtered, output_format=output_format))

    if inplace:
        output_file.close()


@click.command('check', help=c_help.params.check)
@click_settings_file('all')
def check(settings):
    construct_settings(settings, process=False)


@click.group('params', help=c_help.params.command)
def params(*args, **kwargs):
    pass


params.add_command(show, 'show')
params.add_command(check, 'check')
