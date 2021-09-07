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
from attrdict import AttrDict
from sqsgenerator.io import dumps, to_dict
from operator import attrgetter as attr, itemgetter as item
from sqsgenerator.commands.help import parameter_help as help, command_help
from sqsgenerator.settings import construct_settings, process_settings, defaults
from sqsgenerator.commands.common import click_settings_file, error, pretty_print
from sqsgenerator.public import pair_analysis, available_species, extract_structures, expand_sqs_results


species_to_ordinal = dict(map(attr('symbol', 'Z'), available_species()))


def foreach(f, iterable, unpack=False):
    for x in iterable:
        f(*(x if unpack else (x,)))


def map_values(f, d: dict, factory=dict):
    return factory({k: f(v) for k, v in d.items()})


def format_parameters(settings, result):
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


@click.command('analyse', help=command_help.analyse)
@click.option('--output-format', '-of', type=click.Choice(['yaml', 'json', 'pickle', 'native']), default='native',
              help=help.output_format)
@click.option('--params', '-p', is_flag=True, help=help.dump_params)
@click_settings_file(process={'structure'}, ignore={'which'}, default_name='sqs.result.yaml')
def analyse(settings, output_format, params):
    if 'configurations' not in settings:
        error('The input document must contain a "configurations" key', prefix='KeyError')
    num_configurations = len(settings.configurations)

    if num_configurations < 1:
        error('Your input does not contain any input configurations')

    # extract the defines sublattice from the total structure
    structures = map_values(lambda s: s[settings.which], extract_structures(settings))

    # set one of the actual computes structures into settings, to get proper default values
    first_structure = next(iter(structures.values()))

    # a new clone of the actual settings object is create -> we do not want to loose the old -> TODO: find a reason why
    analyse_settings = AttrDict(settings.copy())
    analyse_settings.update(structure=first_structure)
    analyse_settings.update(which=defaults.which(analyse_settings))

    include_fields = ('objective', 'parameters', 'configuration')
    analyse_settings = process_settings(analyse_settings)
    # carry out the actual pair analysis by overwriting the structure in the current settings
    analyzed = map_values(lambda s: pair_analysis(construct_settings(analyse_settings, False, structure=s)), structures)
    document = expand_sqs_results(analyse_settings, list(analyzed.values()), include=include_fields, inplace=params)

    if 'structure' in document:
        del document['structure']
    if 'which' in document:
        del document['which']
    if output_format == 'native':
        all_renderables = []
        for rank, result in map_values(AttrDict, document.configurations).items():
            renderables = list(format_parameters(analyse_settings, AttrDict(result)))
            renderables.insert(0, Text(f'Parameters:{os.linesep}', style='bold'))
            renderables.insert(0, Text.assemble(
                ('Configuration: ', 'bold'), (f'{result.configuration}', 'bold cyan'))
            )
            renderables.insert(0, Text.assemble(('Objective: ', 'bold'), (f'{result.objective}', 'bold cyan')))
            renderables.insert(0, Text.assemble(('Rank: ', 'bold'), (f'{rank}', 'bold cyan')))
            all_renderables.extend(renderables)
        pretty_print(*all_renderables)
    else:
        sys.stdout.buffer.write(dumps(to_dict(document), output_format=output_format))
