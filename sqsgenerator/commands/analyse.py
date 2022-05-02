"""
Analyse CLI command - Calculates SRO parameters, rank and objectives. Intended use is to analyze already existing
structures
"""

import os
import sys
import click
import functools
from rich import box
from rich.text import Text
from rich.table import Table
from sqsgenerator.settings import process_settings
from sqsgenerator.fallback.attrdict import AttrDict
from operator import attrgetter as attr, itemgetter as item
from sqsgenerator.public import available_species, sqs_analyse
from sqsgenerator.io import dumps, to_dict, read_structure_from_file
from sqsgenerator.commands.common import read_settings_file, pretty_print
from sqsgenerator.commands.help import parameter_help as help, command_help


species_to_ordinal = dict(map(attr('symbol', 'Z'), available_species()))


def foreach(f, iterable, unpack=False):
    for x in iterable:
        f(*(x if unpack else (x,)))


def map_values(f, d: dict, factory=dict):
    return factory({k: f(v) for k, v in d.items()})


def filter_dict(d, ignore=frozenset(), factory=dict):
    return factory({k: v for k, v in d.items() if k not in ignore})


def wrap_in_iterable(o, factory=tuple):
    return factory((o,))


def read_structure_file(file_name, reader):
    return read_structure_from_file(AttrDict(
        structure=dict(file=file_name),
        reader=reader
    ))


def first(iterable):
    return next(iter(iterable))


def override(b: dict, factory=AttrDict, **kwargs):
    return factory({**b, **kwargs})


def make_default(f, **kwargs):
    return f(AttrDict(kwargs))


def format_parameters(settings, result: AttrDict):
    """
    Nicely formats SQS parameters into a list of tables
    """
    parameters = result.parameters
    species = sorted(settings.structure.unique_species, key=species_to_ordinal.get)

    def make_table(shell, sw, sros, fmt='{0:.3f}'):
        table = Table(title=f'Shell {shell}, weight = {sw:.2f}', box=box.HORIZONTALS)
        table.add_column('Species', justify='center', style='bold')
        make_species_col = functools.partial(table.add_column, justify='center')
        foreach(make_species_col, species)
        for el, pars in zip(species, sros):
            table.add_row(el, *map(fmt.format, pars))
        return table

    sorted_shell_weights = sorted(settings.shell_weights.items(), key=item(0))

    return (make_table(shell, sw, sros) for (shell, sw), sros in zip(sorted_shell_weights, parameters))


def render_sqs_analyse_result(rank, settings, result: AttrDict):
    renderables = list(format_parameters(settings, result))
    renderables.insert(0, Text(f'Parameters:{os.linesep}', style='bold'))
    renderables.insert(0, Text.assemble(
        ('Configuration: ', 'bold'), (f'{result.configuration}{os.linesep}', 'bold cyan'))
                       )
    renderables.insert(0, Text.assemble(('Objective: ', 'bold'), (f'{result.objective}{os.linesep}', 'bold cyan')))
    renderables.insert(0, Text.assemble(('Rank: ', 'bold'), (f'{rank}{os.linesep}', 'bold cyan')))
    return renderables


@click.command('analyse', help=command_help.analyse)
@click.option('--settings', '-s', type=click.Path(dir_okay=False, allow_dash=True))
@click.option('--reader', '-r', type=click.Choice(['ase', 'pymatgen']), default='ase', help=help.writer)
@click.option('--output-format', '-of', type=click.Choice(['yaml', 'json', 'pickle', 'native']), default='native',
              help=help.output_format)
@click.argument('input-files', type=click.Path(dir_okay=False, allow_dash=True), nargs=-1)
def analyse(input_files, settings, reader, output_format):

    # read optional setting file which is passed to sqs_analyse, settings will
    # be applied to each of the input-structure files
    non_eligible_keys = frozenset({'structure', 'composition'})
    settings = {} if settings is None else filter_dict(read_settings_file(settings), non_eligible_keys, AttrDict)

    # read each of the structure file and wrap it in an iterable -> iterable is necessary for compat. with sqs_analyse
    input_structures = map(lambda f: wrap_in_iterable(read_structure_file(f, reader)), input_files)

    document = {}

    for file_name, structures in zip(input_files, input_structures):
        first_structure = first(structures)

        local_settings = process_settings(override(settings, structure=first_structure))
        local_settings = filter_dict(local_settings, non_eligible_keys, AttrDict)
        # we need to keep the input to be able to render it nicely
        document[file_name] = dict(
            result=sqs_analyse(structures, local_settings, process=False),
            settings=override(local_settings, structure=first_structure)
        )

    if output_format == 'native':
        all_renderables = []
        for file_name, data in document.items():
            settings = data.get('settings')
            all_renderables.append(Text(f'File: {file_name}{os.linesep}', style='bold red underline'))
            for rank, result in data.get('result').items():
                all_renderables.extend(render_sqs_analyse_result(rank, settings, AttrDict(result)))
        pretty_print(*all_renderables)
    else:
        document = {file_name: data.get('result') for file_name, data in document.items()}
        sys.stdout.buffer.write(dumps(to_dict(document), output_format=output_format))
