
import click
import sqsgenerator.core
from sqsgenerator.commands.common import click_settings_file, pretty_print


@click.command('total-permutations')
@click_settings_file({'structure'})
def total_permutations(settings):
    permutations = sqsgenerator.core.total_permutations(settings.structure)
    pretty_print(permutations)
    return permutations


@click.command('shell-distances')
@click_settings_file({'atol', 'rtol', 'structure'})
def shell_distances(settings):
    distances = sqsgenerator.core.default_shell_distances(settings.structure, settings.atol, settings.rtol)
    pretty_print(distances)
    return distances


@click.command('rank')
@click_settings_file({'structure'})
def rank_structure(settings):
    rank = pretty_print(sqsgenerator.core.rank_structure(settings.total_structure if settings.is_sublattice else settings.structure))
    pretty_print(rank)
    return rank


@click.group('compute')
def compute():
    pass


compute.add_command(rank_structure, 'rank')
compute.add_command(shell_distances, 'shell-distances')
compute.add_command(total_permutations, 'total-permutations')
