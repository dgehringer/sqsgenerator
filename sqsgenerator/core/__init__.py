import logging
import typing as T
from sqsgenerator.core.data import Structure, available_species
from sqsgenerator.core.iteration import IterationMode, IterationSettings
from sqsgenerator.core.utils import default_shell_distances


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
