import io
import click
import attrdict
import typing as T
import functools
import collections.abc
from sqsgenerator.settings import process_settings, parameter_list
from sqsgenerator.compat import have_feature, get_module, require, Feature as F


def ensure_iterable(o: T.Any, exclude=(str, bytes, bytearray), factory=set):
    if isinstance(o, collections.abc.Iterable) or isinstance(o, exclude): return o
    else: return factory((o,))


def click_settings_file(process=None):

    def _decorator(f: T.Callable):

        @functools.wraps(f)
        @click.argument('filename', type=click.Path(exists=True))
        @click.option('--input-format', '-if', type=click.Choice(['yaml', 'json', 'pickle']), default='yaml')
        def _dummy(filename, input_format, *args, **kwargs):
            settings = read_settings_file(filename, format=input_format)
            nonlocal process
            if process is not None:
                param_list = parameter_list()
                if 'all' in process: process = param_list
                else: assert all(p in param_list for p in process)
                settings = process_settings(settings, params=process)
            return f(settings, *args, **kwargs)

        return _dummy

    return _decorator


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