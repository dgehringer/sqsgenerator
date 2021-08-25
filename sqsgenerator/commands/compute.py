
import time
import click
import numpy as np
import collections
from math import isclose
from sqsgenerator.settings import construct_settings
from sqsgenerator.core import total_permutations as total_permutations_, default_shell_distances, rank_structure, IterationMode, pair_sqs_iteration
from sqsgenerator.commands.common import click_settings_file, pretty_print


@click.command('total-permutations')
@click_settings_file({'structure', 'mode', 'iterations'})
def total_permutations(settings):
    permutations = total_permutations_(settings.structure) if settings.mode == IterationMode.systematic else settings.iterations
    pretty_print(permutations)
    return permutations


@click.command('shell-distances')
@click_settings_file({'atol', 'rtol', 'structure'})
def shell_distances(settings):
    distances = default_shell_distances(settings.structure, settings.atol, settings.rtol)
    pretty_print(distances)
    return distances


@click.command('rank')
@click_settings_file({'structure'})
def rank_structure(settings):
    rank = pretty_print(rank_structure(settings.total_structure if settings.is_sublattice else settings.structure))
    pretty_print(rank)
    return rank


@click.group('compute')
def compute():
    pass


def format_seconds(seconds: float) -> str:
    minute = 60
    hour = 60*minute
    day = 24*hour
    units = collections.OrderedDict([
        (365*day, 'year'),
        (7*day, 'week'),
        (day, 'day'),
        (hour, 'hour'),
        (minute, 'minute')
    ])
    result = []
    for uval, label in units.items():
        if seconds > uval:
            q, r = divmod(seconds, uval)
            result.append(f'{q:.0f} {label}{"s" if not isclose(q, 1.0) else ""}')
            seconds -= q*uval
    if not isclose(seconds, 0.0):
        result += ['and', f'{seconds:.3f} seconds']
    return ' '.join(result)


@click.command('estimated-time')
@click.option('--verbose', '-v', is_flag=True, default=False)
@click_settings_file('all')
def estimate_time( settings, verbose):
    have_random_mode = settings.mode == IterationMode.random
    num_iterations = settings.iterations if have_random_mode else total_permutations_(settings.structure)
    num_test_iterations = 100000
    default_guess_settings = dict(iterations=num_test_iterations, mode=IterationMode.random)
    settings.update(default_guess_settings)
    iteration_settings = construct_settings(settings, False)

    t0 = time.time()
    _, timings = pair_sqs_iteration(iteration_settings)
    tend = time.time()
    duration = tend - t0
    num_rank = 0
    thread_timings = timings[num_rank]
    average_thread_time = np.average(thread_timings)
    num_threads = len(thread_timings)
    pure_loop_time = 1e-6*average_thread_time*num_test_iterations/num_threads
    overhead = duration - pure_loop_time
    estimated_seconds = num_iterations * average_thread_time / (num_threads * 1e6)

    pretty_print(f'It will take me roughly {format_seconds(estimated_seconds)} to compute {num_iterations} iterations (on {num_threads} threads)')
    if verbose:
        pretty_print(f'The test was carried out with {num_threads} threads')
        pretty_print(f'On average it takes me {average_thread_time / num_threads:.3f} µs to analyze one configuration ({average_thread_time:.3f} µs per thread)')
        pretty_print(f'This is a throughput of {1e6 / (average_thread_time / num_threads):.0f} configurations/s ({1e6 / average_thread_time:.0f} configurations/s per thread)')
        pretty_print(f'The initialization/finalization overhead was {overhead:.2f} seconds')

    return estimated_seconds

compute.add_command(rank_structure, 'rank')
compute.add_command(shell_distances, 'shell-distances')
compute.add_command(total_permutations, 'total-permutations')
compute.add_command(estimate_time, 'estimated-time')
