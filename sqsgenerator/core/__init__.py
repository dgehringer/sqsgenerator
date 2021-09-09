import logging
import operator
import functools
import frozendict
import typing as T
from sqsgenerator.core.core import SQSResult
from sqsgenerator.core.core import __version__, __features__
from sqsgenerator.core.core import set_log_level, pair_sqs_iteration, pair_analysis
from sqsgenerator.core.core import IterationMode, IterationSettings, BoostLogLevel
from sqsgenerator.core.structure import Structure, structure_to_dict, Atom, make_supercell
from sqsgenerator.core.core import default_shell_distances, total_permutations, rank_structure, atoms_from_numbers, \
    atoms_from_symbols, available_species, symbols_from_z

__all__ = [
    '__version__',
    '__features__',
    'set_log_level',
    'pair_sqs_iteration',
    'IterationMode',
    'IterationSettings',
    'BoostLogLevel',
    'pair_analysis',
    'Structure',
    'structure_to_dict',
    'get_function_logger',
    'Atom',
    'make_supercell',
    'SQSResult',
    'default_shell_distances',
    'total_permutations',
    'rank_structure',
    'atoms_from_numbers',
    'atoms_from_symbols',
    'available_species',
    'symbols_from_z'
]

attr = operator.attrgetter
item = operator.itemgetter
method = operator.methodcaller
partial = functools.partial

TRACE = 5
logging.addLevelName(TRACE, 'TRACE')

log_levels = frozendict.frozendict(
    trace=TRACE,
    debug=logging.DEBUG,
    info=logging.INFO,
    warning=logging.WARNING,
    error=logging.ERROR
)


def set_core_log_level(level: int = logging.WARNING) -> T.NoReturn:
    level_map = {
        TRACE: BoostLogLevel.trace,
        logging.DEBUG: BoostLogLevel.debug,
        logging.INFO: BoostLogLevel.info,
        logging.WARNING: BoostLogLevel.warning,
        logging.ERROR: BoostLogLevel.error
    }
    set_log_level(level_map.get(level))


def get_function_logger(f: T.Callable) -> logging.Logger:
    return logging.getLogger(f.__module__ + '.' + f.__name__)


set_core_log_level()