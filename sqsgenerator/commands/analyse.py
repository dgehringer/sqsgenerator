import sys
import click
import tabulate
from attrdict import AttrDict
from operator import attrgetter as attr
from sqsgenerator.io import dumps, to_dict
from sqsgenerator.main import pair_analysis, available_species, extract_structures, expand_sqs_results
from sqsgenerator.commands.common import click_settings_file, error
from sqsgenerator.settings import construct_settings, process_settings


species_to_ordinal = dict(map(attr('symbol', 'Z'), available_species()))


def map_values(f, d: dict, factory=dict):
    return factory({k: f(v) for k, v in d.items()})


def format_parameters(settings, result, tablefmt='fancy_grid'):
    structure = settings.structure
    parameters = result.parameters(settings.target_objective.shape)
    species = sorted(structure.unique_species, key=species_to_ordinal.get)
    return [tabulate.tabulate(sros, headers=species, tablefmt=tablefmt, showindex=species) for sros in parameters]


@click.command('analyse')
@click.option('--output-format', '-of', type=click.Choice(['yaml', 'json', 'pickle']), default='yaml')
@click.option('--params', '-p', is_flag=True)
@click.option('--include', '-i', type=click.Choice(['parameters', 'timings', 'objective']), multiple=True)
@click_settings_file(process=None)
def analyse(settings, include, output_format, params):
    settings = AttrDict(settings)
    if 'configurations' not in settings:
        error('The input document must contain a "configurations" key', prefix='KeyError')
    num_configurations = len(settings.configurations)

    if num_configurations < 1:
        error('Your input does not contain any input configurations')

    have_sublattice = 'which' in settings

    structures = map_values(
        lambda s: s[settings.which] if have_sublattice else s,
        extract_structures(settings)
    )

    # get first structure to compute default values
    first_structure = next(iter(structures.values()))
    settings['structure'] = first_structure

    settings = process_settings(settings, ignore=('composition',))

    def make_iterations_settings(s):
        cp = AttrDict(settings.copy())
        cp['structure'] = s
        return construct_settings(cp, False)

    analyzed = map_values(lambda s: pair_analysis(make_iterations_settings(s)), structures)

    document = expand_sqs_results(settings, list(analyzed.values()), include=include, inplace=params)
    if 'structure' in document:
        del document['structure']
    sys.stdout.buffer.write(dumps(to_dict(document), output_format=output_format))

