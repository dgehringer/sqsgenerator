import logging
import operator
import functools
import frozendict
import typing as T
from sqsgenerator.core.core import SQSResult, Atom
from sqsgenerator.core.core import __version__, __features__
from sqsgenerator.core.core import set_log_level, pair_sqs_iteration, pair_analysis
from sqsgenerator.core.core import IterationMode, IterationSettings, BoostLogLevel
from sqsgenerator.core.structure import Structure, structure_to_dict, make_supercell
from sqsgenerator.core.core import default_shell_distances, atoms_from_numbers, atoms_from_symbols, available_species, \
    symbols_from_z, z_from_symbols, build_configuration, ALL_SITES, make_rank as make_rank_, \
    rank_structure as rank_structure_, total_permutations as total_permutations_, \
    compute_prefactors as compute_prefactors_

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
    'symbols_from_z',
    'z_from_symbols',
    'build_configuration',
    'ALL_SITES',
    'make_rank',
    'rank_structure',
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


set_core_log_level(logging.WARNING)


def total_permutations(struct: Structure, which: T.Optional[T.Iterable[int]] = None) -> int:
    """
    Compute the total number of total permutations, by looking at the configuration of {struct}. The number of
    of permutations are given by the multinomial coefficient. Let :math:`N_1, N_2, \ldots, N_m` the number of the
    atoms of the :math:`m^{\\text{th}}` species. Therefore, one can generate

    .. math::

        N_{\\text{perms}} = \dfrac{N!}{\prod_m^M N_m} \quad \\text{where} \quad \sum_m^M N_m = N

    permutations in total

    :param struct: the structure with the configuration to calculate the number of total permutations
    :type struct: Structure
    :param which: the indices of the lattice positions to choose (default is ``None``)
    :type which: Optional[Iterable[int]]
    :return: the rank of the structure of the sub-lattice selected by {which}
    :rtype: int
    """
    which = tuple(range(len(struct))) if which is None else which

    return total_permutations_(struct[which])


def make_rank(struct: Structure, rank: int, configuration: T.Optional[T.Iterable[str]] = None,
              which: T.Optional[T.Iterable[int]] = None) -> Structure:
    """
    Uses the lattice and positions from {struct} and generates the {rank}-th permutation (in lexicographical order)
    of {configuration} and creates a new structure with it. If {which} is given, the selected lattices positions
    are used, therefore the length of {configuration} must match the length of {which}

    :param struct: the basis structure from which the new structure will be created
    :type struct: Structure
    :param rank: the index of the permutation sequence
    :type rank: int
    :param configuration: the species sequence from which the {rank}-th permutation sequence. Use this parameter when
        you want to create a rank and simultaneously want to distribute new atomic species (default is ``None``)
    :type configuration: Optional[Iterable[str]]
    :param which: the indices of the lattice positions to choose (default is ``None``)
    :type which: Optional[Iterable[int]]
    :raises IndexError: if the length of {which} does not match the length of {configuration}
    :raises ValueError: if the rank is less than 1 or greater than the number of :py:func:`total_permutations`
    :return: the structure with {rank}-th permutation sequence as configuration
    :rtype: Structure
    """

    configuration = struct.symbols.tolist() if configuration is None else configuration

    msg = f"the rank of a configuration must me 1 <= rank <= total_permutations() = " \
          f"{total_permutations(struct.slice_with_species(configuration, which))}"
    if rank <= 0:
        raise ValueError(msg)

    which = tuple(range(len(struct))) if which is None else which
    try:
        unranked_configuration = make_rank_(configuration, rank)
    except IndexError:
        # the C++ extension raises an IndexError, we want to propagate it as a Value error
        raise ValueError(msg)

    return struct.with_species(unranked_configuration, which=which)


def rank_structure(struct: Structure, which: T.Optional[T.Iterable[int]] = None) -> int:
    """
    Computes the index (in lexicographical order) of the configuration sequence.
    If {which} is specified only those sites are selected to compute the permutation rank

    :param struct: the structure which carries the configuration sequence
    :type struct: Structure
    :param which: the indices of the lattice positions to choose (default is ``None``)
    :type which: Optional[Iterable[int]]
    :return: the index of the configuration sequence
    :rtype: int
    """
    which = tuple(range(len(struct))) if which is None else which
    return rank_structure_(struct[which])


def compute_prefactors(structure: Structure, composition: dict, shell_wights: dict, atol: float = 1e-3, rtol: float = 1e-5):
    return compute_prefactors_(structure, composition, shell_wights, atol, rtol)
