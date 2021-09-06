"""
Because on the ReadTheDocs Server we cannot import the core extension, because a libpython*so which the core.so
is linked against is not found, this module fakes the core.so such that imports works
"""


__version__ = (1, 2, 3, 4)
__features__ = {}


class SQSResult:
    pass


class IterationSettings:
    pass


class BoostLogLevel:
    trace = 1
    debug = 2
    info = 3
    warning = 4
    error = 5


class IterationMode:
    random = 'random'
    systematic = 'systematic'


class Structure:
    pass


class Atom:

    def __init__(self, z, symbol):
        self.Z = z
        self.symbol = symbol


def set_log_level(*args, **kwargs):
    pass


def pair_sqs_iteration(*args, **kwargs):
    pass


def pair_analysis(*args, **kwargs):
    pass


def default_shell_distances(*args, **kwargs):
    pass


def total_permutations(*args, **kwargs):
    pass


def rank_structure(*args, **kwargs):
    pass


def atoms_from_numbers(*args, **kwargs):
    pass


def atoms_from_symbols(*args, **kwargs):
    pass


def available_species(*args, **kwargs):
    return [Atom(1, 'H')]


def symbols_from_z(*args, **kwargs):
    pass