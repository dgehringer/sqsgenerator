import sys

import click
import attrdict
import typing as T
from sqsgenerator.compat import require, Feature as F, have_feature, get_module
from sqsgenerator.settings import process_settings, settings_to_dict, parameter_list


def error(message, exc_type=click.Abort, raise_exc=True, style=None):
    style = style or dict(fg='red', bold=True)
    click.secho(message, **style)
    if raise_exc: raise exc_type(message)


@require(F.yaml, F.json, condition=any)
def read_settings_file(path, format='yaml') -> T.Optional[attrdict.AttrDict]:
    f = F(format)
    readers = {
        F.json: 'loads',
        F.pickle: 'loads',
        F.yaml: 'safe_load'
    }
    if not have_feature(f):
        error(f'The package "{format}" is not installed, consider to install it with')
    reader = getattr(get_module(f), readers[f])
    try:
        with open(path, 'r') as settings_file:
            content = settings_file.read()
    except FileNotFoundError:
        error(f'The settings file "{path}" does not exist')
        return
    try:
        data = attrdict.AttrDict(reader(content))
    except Exception as e:
        error(f'While reading the file "{path}", a "{type(e).__name__}" occurred. Maybe the file has the wrong format. I was expecting a "{format}"-file. You can specify a different input-file format using the "--format" option')
        return
    return data


@click.command('process')
@click.argument('filename', type=click.Path(exists=True))
@click.option('--input-format', '-if', type=click.Choice(['yaml', 'json', 'pickle']), default='yaml')
@click.option('--param', '-p', type=click.Choice(parameter_list()), default=None, multiple=True)
@click.option('--output-format', '-of', type=click.Choice(['yaml', 'json', 'pickle', 'native']), default='native')
def process(filename, input_format, param, output_format):
    settings = read_settings_file(filename, format=input_format)
    parameters_to_process = set(param or parameter_list())
    processed = process_settings(settings, params=parameters_to_process)

    filtered = {k : v for k, v in settings_to_dict(processed).items() if k in parameters_to_process}

    if output_format == 'native':
        import rich
        rich.print(filtered)
    else:
        f = F(output_format)
        if not have_feature(f):
            error(f'The package "{format}" is not installed, consider to install it with')
        dumpers = {
            F.json: 'dumps',
            F.pickle: 'dumps',
            F.yaml: 'safe_dump'
        }
        dumper = getattr(get_module(f), dumpers[f])
        content = dumper(filtered)
        if f == F.pickle: sys.stdout.buffer.write(content)
        else: print(content)
