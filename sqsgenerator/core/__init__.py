import logging
import typing as T
from sqsgenerator.core.data import Structure, available_species
from sqsgenerator.core.iteration import IterationMode, IterationSettings, __version__, __features__, BoostLogLevel, set_log_level
from sqsgenerator.core.utils import default_shell_distances, total_permutations, rank_structure

TRACE = 5
logging.addLevelName(TRACE, 'TRACE')


def set_core_log_level(level: int = logging.WARNING) -> T.NoReturn:
    level_map = {
        TRACE: BoostLogLevel.trace,
        logging.DEBUG: BoostLogLevel.debug,
        logging.INFO: BoostLogLevel.info,
        logging.WARNING: BoostLogLevel.warning,
        logging.ERROR: BoostLogLevel.error
    }
    set_log_level(level_map.get(level))


class BadSettings(Exception):
    pass


def get_function_logger(f: T.Callable) -> logging.Logger:
    return logging.getLogger(f.__module__ + '.' + f.__name__)


def setup_logging():
    try:
        import rich.logging
    except ImportError: pass
    else:
        logging.basicConfig(
            level="NOTSET",
            format="%(message)s",
            datefmt="[%X]",
            handlers=[rich.logging.RichHandler(rich_tracebacks=True)]
    )

set_core_log_level()