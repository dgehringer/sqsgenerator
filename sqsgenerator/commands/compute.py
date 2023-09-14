"""
Compute quantities which help optimizing the input settings
"""

import rich
import time
import click
import numpy as np
import collections
from math import isclose
from functools import partial
from sqsgenerator.commands.help import command_help as c_help
from sqsgenerator.settings import construct_settings, build_structure
from sqsgenerator.commands.common import click_settings_file, pretty_print, error
from sqsgenerator.settings.defaults import default_shell_distances, DEFAULT_PAIR_HIST_BIN_WIDTH
from sqsgenerator.core import total_permutations as total_permutations_, \
    rank_structure as rank_structure_core, IterationMode, pair_sqs_iteration


@click.command('total-permutations', help=c_help.compute.total_permutations)
@click_settings_file({'structure', 'mode', 'iterations', 'composition'})
def total_permutations(settings):
    structure = build_structure(settings.composition, settings.structure[settings.which])
    permutations = total_permutations_(structure) \
        if settings.mode == IterationMode.systematic else settings.iterations
    pretty_print(permutations)
    return permutations


def in_bounds(min_val: float, max_val: float, val: float) -> bool:
    if min_val > max_val:
        min_val, max_val = max_val, min_val
    return (isclose(val, min_val) or  isclose(val, max_val)) or (min_val <= val <= max_val)


@click.command('shell-distances', help=c_help.compute.shell_distances)
@click_settings_file({'atol', 'rtol', 'structure', 'which', 'composition'})
def shell_distances(settings):
    distances = default_shell_distances(settings)
    pretty_print(distances)
    return distances


@click.command('rank', help=c_help.compute.rank)
@click_settings_file({'structure'})
def rank_structure(settings):
    rank = pretty_print(rank_structure_core(settings.structure))
    pretty_print(rank)
    return rank


@click.group('compute', help=c_help.compute.command)
def compute():
    pass


def format_seconds(seconds: float) -> str:
    """
    Runtimes can be really long, due to exponentially growing configurational space, therefore those h
    """
    minute = 60
    hour = 60*minute
    day = 24*hour
    year = 365 * day
    units = collections.OrderedDict([
        (230000000*year, 'galactic year'),
        (1000*year, 'millenium'),
        (10*year, 'decade'),
        (year, 'year'),
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


@click.command('estimated-time', help=c_help.compute.estimated_time)
@click.option('--verbose', '-v', is_flag=True, default=False, help='Print very detailed infos')
@click_settings_file('all')
def estimate_time(settings, verbose):
    have_random_mode = settings.mode == IterationMode.random
    structure = build_structure(settings.composition, settings.structure[settings.which])
    num_iterations = settings.iterations if have_random_mode else total_permutations_(structure)
    num_test_iterations = 100000
    default_guess_settings = dict(iterations=num_test_iterations, mode=IterationMode.random)
    settings.update(default_guess_settings)
    iteration_settings = construct_settings(settings, process=False, structure=settings.structure[settings.which])

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

    pretty_print(f'It will take me roughly {format_seconds(estimated_seconds)} '
                 f'to compute {num_iterations} iterations (on {num_threads} threads)')
    if verbose:
        pretty_print(f'The test was carried out with {num_threads} threads')
        pretty_print(f'On average it takes me {average_thread_time / num_threads:.3f} µs '
                     f'to analyze one configuration ({average_thread_time:.3f} µs per thread)')
        pretty_print(f'This is a throughput of {1e6 / (average_thread_time / num_threads):.0f} '
                     f'configurations/s ({1e6 / average_thread_time:.0f} configurations/s per thread)')
        pretty_print(f'The initialization/finalization overhead was {overhead:.2f} seconds')

    return estimated_seconds


compute.add_command(rank_structure, 'rank')
compute.add_command(shell_distances, 'shell-distances')
compute.add_command(total_permutations, 'total-permutations')
compute.add_command(estimate_time, 'estimated-time')
