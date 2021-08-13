
import sys
import click
from sqsgenerator.commands.common import read_settings_file, pretty_print, dumps, click_settings_file
from sqsgenerator.settings import process_settings, settings_to_dict, parameter_list, construct_settings


@click.command('show')
@click.option('--param', '-p', type=click.Choice(parameter_list()), default=None, multiple=True)
@click.option('--output-format', '-of', type=click.Choice(['yaml', 'json', 'pickle', 'native']), default='native')
@click_settings_file('all')
def show(settings, param, output_format):
    parameters_to_process = set(param or parameter_list())
    processed = process_settings(settings, params=parameters_to_process)
    filtered = {k : v for k, v in settings_to_dict(processed).items() if k in parameters_to_process}
    if output_format == 'native': pretty_print(filtered)
    else: sys.stdout.buffer.write(dumps(filtered, output_format=output_format))


@click.command('check')
@click_settings_file('all')
def check(settings):
    construct_settings(settings, process=False)


@click.group('params')
def params(*args, **kwargs):
    pass


params.add_command(show, 'show')
params.add_command(check, 'check')