import io
import pprint
import sys

import click
import attrdict
import typing as T
from sqsgenerator.compat import require, Feature as F, have_feature, get_module
from sqsgenerator.settings import process_settings, settings_to_dict, parameter_list, construct_settings


def error(message, exc_type=click.Abort, raise_exc=True, style=None):
    style = style or dict(fg='red', bold=True)
    click.secho(message, **style)
    if raise_exc: raise exc_type(message)


def pretty_print(*objects, **kwargs):
    buf = io.StringIO()
    if have_feature(F.rich):
        get_module(F.rich).print(*objects, **kwargs)
    else:
        from pprint import pprint
        for o in objects: pprint(o, stream=buf, **kwargs)
    return buf.getvalue()


@require(F.json, F.yaml, F.pickle, condition=any)
def dumps(o: dict, output_format='yaml'):
    f = F(output_format)
    if not have_feature(f):
        error(f'The package "{format}" is not installed, consider to install it with')

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
        error(f'The package "{format}" is not installed, consider to install it with')
    reader = getattr(get_module(f), readers[f])
    try:
        with open(path, 'r' if f != F.pickle else 'rb') as settings_file:
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


@click.command('show')
@click.argument('filename', type=click.Path(exists=True))
@click.option('--input-format', '-if', type=click.Choice(['yaml', 'json', 'pickle']), default='yaml')
@click.option('--param', '-p', type=click.Choice(parameter_list()), default=None, multiple=True)
@click.option('--output-format', '-of', type=click.Choice(['yaml', 'json', 'pickle', 'native']), default='native')
def show(filename, input_format, param, output_format):
    settings = read_settings_file(filename, format=input_format)
    parameters_to_process = set(param or parameter_list())
    processed = process_settings(settings, params=parameters_to_process)
    filtered = {k : v for k, v in settings_to_dict(processed).items() if k in parameters_to_process}
    if output_format == 'native': pretty_print(filtered)
    else: sys.stdout.buffer.write(dumps(filtered, output_format=output_format))


@click.command('check')
@click.argument('filename', type=click.Path(exists=True))
@click.option('--input-format', '-if', type=click.Choice(['yaml', 'json', 'pickle']), default='yaml')
def check(filename, input_format):
    settings = read_settings_file(filename, format=input_format)
    construct_settings(settings)


@click.group('params')
def params(*args, **kwargs):
    pass


params.add_command(show, 'show')
params.add_command(check, 'check')