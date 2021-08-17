
import io
import os
import sys
import click
import attrdict
import typing as T
import functools
import collections.abc
from sqsgenerator.settings import process_settings, parameter_list, BadSettings
from sqsgenerator.compat import have_feature, get_module, require, Feature as F


def ensure_iterable(o: T.Any, exclude=(str, bytes, bytearray), factory=set):
    if isinstance(o, collections.abc.Iterable) or isinstance(o, exclude): return o
    else: return factory((o,))


def error(message, exc_type=click.Abort, raise_exc=True, prefix=None):
    message = pretty_print(message, show=False)
    if prefix is not None:
        prefix = click.style(prefix, fg='red', bold=True, underline=True)
        message = f'{prefix}: {message}'
    click.echo(message)
    if raise_exc: raise exc_type(message)


def pretty_print(*objects, show=True, paginate='auto', **kwargs):
    """
    Formats an object nicely. Tries to use "rich" if available. Falls back to pprint.pprint alternatively
    :param objects: the objects to print
    :param show: if `show` is true the output will be printed to `sys.stdout` (default is `True`)
    :type show: bool
    :param paginate: paginated output, allowed is `True`, `False` and `'auto'` (default is `'auto'`)
    :type paginate: bool or str
    :param kwargs: keyword args passed on to rich.console.print  or pprint.pprint
    :return: the formatted output if `show` is `False` else `None` is returned
    :rtype: str of None
    """
    buf = io.StringIO()
    if have_feature(F.rich):
        console = get_module(F.rich).get_console()
        with console.capture() as capture:
            console.print(*objects, **kwargs)
        buf.write(capture.get())
        console_height = console.height
    else:
        from pprint import pprint
        for o in objects: pprint(o, stream=buf, **kwargs)
        console_height = 25
    string = buf.getvalue()
    if paginate == 'auto': paginate = string.count(os.linesep) >= console_height
    printer = functools.partial(click.echo, file=sys.stdout, nl=False) if not paginate else click.echo_via_pager
    return printer(buf.getvalue()) if show else buf.getvalue()


def make_help_link(parameter: str):
    section_permalink = parameter.replace('_', '-')
    base_url = 'https://sqsgenerator.readthedocs.io'
    help_page = 'input_parameters.html'
    return f'{base_url}/{help_page}#{section_permalink}'


def exit_on_input_parameter_error(f):

    @functools.wraps(f)
    def _inner(*args, **kwargs):
        try:
            result = f(*args, **kwargs)
        except BadSettings as exc:
            prefix = type(exc).__name__
            raise_first = exc.parameter is None
            error(exc.message, prefix=prefix, raise_exc=raise_first)
            error(f'Maybe the documentation can help you: {make_help_link(exc.parameter)}',
                  prefix=prefix, raise_exc=not raise_first)
        else:
            return result

    return _inner


def click_settings_file(process=None):

    def _decorator(f: T.Callable):

        @functools.wraps(f)
        @click.argument('filename', type=click.Path(exists=True), default='sqs.yaml')
        @click.option('--input-format', '-if', type=click.Choice(['yaml', 'json', 'pickle']), default='yaml')
        def _dummy(filename, input_format, *args, **kwargs):
            settings = read_settings_file(filename, format=input_format)
            settings['file_name'] = filename
            settings['input_format'] = input_format
            nonlocal process
            if process is not None:
                param_list = parameter_list()
                if 'all' in process: process = param_list
                else: assert all(p in param_list for p in process)
                settings = process_settings(settings, params=process)
            return f(settings, *args, **kwargs)

        return exit_on_input_parameter_error(_dummy)

    return _decorator


@require(F.json, F.yaml, F.pickle, condition=any)
def dumps(o: dict, output_format='yaml'):
    f = F(output_format)
    if not have_feature(f): error(f'The package "{format}" is not installed, consider to install it with')

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
        prefix = type(e).__name__
        error(f'While reading the file "{path}", a "{prefix}" occurred. Maybe the file has the wrong format. I was expecting a "{format}"-file. You can specify a different input-file format using the "--format" option', prefix=prefix)
        return
    return data