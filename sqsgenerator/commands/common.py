"""
Utilities which are used by the CLI interface commands
"""

import io
import os
import sys
import rich
import click
import functools
import typing as T
from sqsgenerator.io import read_settings_file
from sqsgenerator.commands.help import parameter_help as help
from sqsgenerator.settings import process_settings, parameter_list, BadSettings


def error(message: str, exc_type=click.Abort, raise_exc: bool = True, prefix: T.Optional[str] = None, **kwargs):
    """
    Prints an error message and abort execution by raising an error
    :param message: the error message to print
    :type message: str
    :param exc_type: the exception type to raise (default is `click.Abort`)
    :type exc_type: type
    :param raise_exc: raise an exception (default is `True`)
    :type raise_exc: True
    :param prefix: a bold red prefix to the error message
    :type prefix: str
    :param kwargs: keyword args are forwarded `to click.echo`
    """
    message = pretty_print(message, show=False)
    if prefix is not None:
        prefix = click.style(prefix, fg='red', bold=True, underline=True)
        message = f'{prefix}: {message}'
    click.echo(message, **kwargs)
    if raise_exc:
        raise exc_type(message)


def pretty_print(*objects: T.Any, show: bool = True, paginate: str = 'auto', **kwargs):
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
    console = rich.get_console()
    with console.capture() as capture:
        console.print(*objects, **kwargs)
    buf.write(capture.get())
    console_height = console.height

    string = buf.getvalue()
    if paginate == 'auto':
        paginate = string.count(os.linesep) >= console_height
    printer = functools.partial(click.echo, file=sys.stdout, nl=False) if not paginate else click.echo_via_pager
    return printer(buf.getvalue()) if show else buf.getvalue()


def make_help_link(parameter: str) -> str:
    """
    Formats a help link pointing to the input parameter documentation
    :param parameter: the input parameter name
    :type parameter: str
    :return: the helplink
    :rtype: str
    """
    section_permalink = parameter.replace('_', '-')
    base_url = 'https://sqsgenerator.readthedocs.io/en/latest'
    help_page = 'input_parameters.html'
    return f'{base_url}/{help_page}#{section_permalink}'


def exit_on_input_parameter_error(f):
    """
    Decorator: Wraps a @parameter decorated function -> catches an eventual `BadSettings` error -> creates a help-link
    for the parameter -> redirects the `BadSettings` into `click.Abort` to stop CLI execution
    """

    @functools.wraps(f)
    def _inner(*args, **kwargs):
        try:
            result = f(*args, **kwargs)
        except BadSettings as exc:
            prefix = type(exc).__name__
            raise_first = exc.parameter is None
            error(exc.message, prefix=prefix, raise_exc=raise_first, nl=False)
            error(f'Maybe the documentation can help you: {make_help_link(exc.parameter)}',
                  prefix=prefix, raise_exc=not raise_first, nl=False)
        else:
            return result

    return _inner


def click_settings_file(process=None, default_name='sqs.yaml', ignore=()):
    """
    Decorator-Factory: A lot of the commands need an input YAML file -> creates a decorator which parses a settings
    file and takes an input format as parameter. The decorator processes the specified file
    :param process: process (parsed) the settings dictionary with sqsgenerator.core (default is `None`)
    :type process: Iterable[str] or None
    :param default_name: if no file path is specified the function will search {default_name} (default is `"sqs.yaml"`)
    :type default_name: str
    :param ignore: list of parameters names which are ignored
    :return: the function wrapped by a `click.option` {input_format} and a `click.argument` {filename} decorator
    :rtype: callable
    """

    def _decorator(f: T.Callable):

        @functools.wraps(f)
        @click.argument('filename', type=click.Path(exists=True), default=default_name)
        @click.option('--input-format', '-if', type=click.Choice(['yaml', 'json', 'pickle']), default='yaml',
                      help=help.input_format)
        def _dummy(*args, filename=default_name, input_format='yaml', **kwargs):
            settings = read_settings_file(filename, format=input_format)
            settings['file_name'] = filename
            settings['input_format'] = input_format
            nonlocal process
            if process is not None:
                param_list = parameter_list()
                if 'all' in process:
                    process = param_list

                settings = process_settings(settings, params=process, ignore=ignore)

            return f(settings, *args, **kwargs)

        return exit_on_input_parameter_error(_dummy)

    return _decorator


pretty_format = functools.partial(pretty_print, show=False)
