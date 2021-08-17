import time
import click
from sqsgenerator.settings import construct_settings
from sqsgenerator.commands.common import click_settings_file
from sqsgenerator.core import pair_sqs_iteration, set_core_log_level, log_levels, atoms_from_numbers
from operator import attrgetter as attr


@click.command('run')
@click.option('--log-level', type=click.Choice(list(log_levels.keys())), default='warning')
@click_settings_file('all')
def run(settings, log_level):

    iteration_settings = construct_settings(settings, False)
    set_core_log_level(log_levels.get(log_level))
    sqs_results, timings = pair_sqs_iteration(iteration_settings)
    print(settings.sublattice.which)
    for result in sqs_results:
        print(result.configuration, type(result.configuration), list(map(attr('Z'), atoms_from_numbers(result.configuration))))

