import sys
import click
import tabulate
from attrdict import AttrDict
from operator import attrgetter as attr
from sqsgenerator.io import dumps, to_dict
from sqsgenerator.main import pair_analysis, available_species, extract_structures, expand_sqs_results, set_core_log_level
from sqsgenerator.commands.common import click_settings_file, error, pretty_print
from sqsgenerator.settings import construct_settings, parameter_list, process_settings, defaults


species_to_ordinal = dict(map(attr('symbol', 'Z'), available_species()))


def map_values(f, d: dict, factory=dict):
    return factory({k: f(v) for k, v in d.items()})


def format_parameters(settings, result, tablefmt='fancy_grid'):
    structure = settings.structure
    parameters = result.parameters(settings.target_objective.shape)
    species = sorted(structure.unique_species, key=species_to_ordinal.get)
    return [tabulate.tabulate(sros, headers=species, tablefmt=tablefmt, showindex=species) for sros in parameters]


@click.command('analyse')
@click.option('--output-format', '-of', type=click.Choice(['yaml', 'json', 'pickle', 'native']), default='native')
@click.option('--params', '-p', is_flag=True)
@click.option('--include', '-i', type=click.Choice(['parameters', 'timings', 'objective']), multiple=True)
@click_settings_file(process={'structure'}, ignore={'which'})
def analyse(settings, include, output_format, params):
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

    analyse_settings = process_settings(analyse_settings, )
    # carry out the actual pair analysis by overwriting the structure in the current settings
    analyzed = map_values(lambda s: pair_analysis(construct_settings(analyse_settings, False, structure=s)), structures)
    document = expand_sqs_results(analyse_settings, list(analyzed.values()), include=include, inplace=params)

    if 'structure' in document:
        del document['structure']
    if 'which' in document:
        del document['which']
    sys.stdout.buffer.write(dumps(to_dict(document), output_format=output_format))

