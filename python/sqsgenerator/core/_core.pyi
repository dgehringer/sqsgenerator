import numpy
from _typeshed import Incomplete
from typing import ClassVar, Iterable, Iterator, overload

__build__: tuple
__version__: tuple
bad_argument: ParseErrorCode
bad_value: ParseErrorCode
chunk_setup: Timing
cif: StructureFormat
comm: Timing
critical: LogLevel
debug: LogLevel
double: Prec
info: LogLevel
interact: SublatticeMode
json_ase: StructureFormat
json_pymatgen: StructureFormat
json_sqsgen: StructureFormat
loop: Timing
naive: ShellRadiiDetection
not_found: ParseErrorCode
out_of_range: ParseErrorCode
peak: ShellRadiiDetection
poscar: StructureFormat
random: IterationMode
single: Prec
split: SublatticeMode
systematic: IterationMode
total: Timing
trace: LogLevel
type_error: ParseErrorCode
undefined: Timing
unknown: ParseErrorCode
warn: LogLevel

class Atom:
    def __init__(self, *args, **kwargs) -> None: ...
    @staticmethod
    def from_symbol(symbol: str) -> Atom: ...
    @staticmethod
    def from_z(ordinal: int) -> Atom: ...
    def __eq__(self, arg0: Atom) -> bool: ...
    def __lt__(self, arg0: Atom) -> bool: ...
    @property
    def atomic_radius(self) -> float: ...
    @property
    def electronegativity(self) -> float: ...
    @property
    def electronic_configuration(self) -> str: ...
    @property
    def mass(self) -> float: ...
    @property
    def name(self) -> str: ...
    @property
    def symbol(self) -> str: ...
    @property
    def z(self) -> int: ...

class AtomPair:
    def __init__(self, *args, **kwargs) -> None: ...
    @property
    def i(self) -> int: ...
    @property
    def j(self) -> int: ...
    @property
    def shell(self) -> int: ...

class Indices:
    def __init__(self) -> None: ...
    def add(self, value: int) -> None: ...
    def contains(self, arg0: int) -> bool: ...
    def empty(self) -> bool: ...
    def size(self) -> int: ...
    def __iter__(self) -> Iterator[int]: ...
    def __len__(self) -> int: ...
    @overload
    def __or__(self, other: list[int]) -> Indices: ...
    @overload
    def __or__(self, other: Indices) -> Indices: ...
    @overload
    def __or__(self, other: Iterable) -> Indices: ...

class IterationMode:
    __members__: ClassVar[dict] = ...  # read-only
    __entries: ClassVar[dict] = ...
    random: ClassVar[IterationMode] = ...
    systematic: ClassVar[IterationMode] = ...
    def __init__(self, value: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class LogLevel:
    __members__: ClassVar[dict] = ...  # read-only
    __entries: ClassVar[dict] = ...
    critical: ClassVar[LogLevel] = ...
    debug: ClassVar[LogLevel] = ...
    info: ClassVar[LogLevel] = ...
    trace: ClassVar[LogLevel] = ...
    warn: ClassVar[LogLevel] = ...
    def __init__(self, value: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class ParseError:
    def __init__(self, *args, **kwargs) -> None: ...
    @property
    def code(self) -> ParseErrorCode: ...
    @property
    def key(self) -> str: ...
    @property
    def msg(self) -> str: ...
    @property
    def parameter(self) -> str | None: ...

class ParseErrorCode:
    __members__: ClassVar[dict] = ...  # read-only
    __entries: ClassVar[dict] = ...
    bad_argument: ClassVar[ParseErrorCode] = ...
    bad_value: ClassVar[ParseErrorCode] = ...
    not_found: ClassVar[ParseErrorCode] = ...
    out_of_range: ClassVar[ParseErrorCode] = ...
    type_error: ClassVar[ParseErrorCode] = ...
    unknown: ClassVar[ParseErrorCode] = ...
    def __init__(self, value: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class Prec:
    __members__: ClassVar[dict] = ...  # read-only
    __entries: ClassVar[dict] = ...
    double: ClassVar[Prec] = ...
    single: ClassVar[Prec] = ...
    def __init__(self, value: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class ShellRadiiDetection:
    __members__: ClassVar[dict] = ...  # read-only
    __entries: ClassVar[dict] = ...
    naive: ClassVar[ShellRadiiDetection] = ...
    peak: ClassVar[ShellRadiiDetection] = ...
    def __init__(self, value: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class SiteDouble:
    def __init__(self, *args, **kwargs) -> None: ...
    def atom(self) -> Atom: ...
    def __eq__(self, arg0: SiteDouble) -> bool: ...
    def __hash__(self) -> int: ...
    def __iter__(self) -> Iterator[object]: ...
    def __lt__(self, arg0: SiteDouble) -> bool: ...
    @property
    def frac_coords(self) -> numpy.ndarray[numpy.float64[3, 1]]: ...
    @property
    def index(self) -> int: ...
    @property
    def specie(self) -> int: ...

class SiteFloat:
    def __init__(self, *args, **kwargs) -> None: ...
    def atom(self) -> Atom: ...
    def __eq__(self, arg0: SiteFloat) -> bool: ...
    def __hash__(self) -> int: ...
    def __iter__(self) -> Iterator[object]: ...
    def __lt__(self, arg0: SiteFloat) -> bool: ...
    @property
    def frac_coords(self) -> numpy.ndarray[numpy.float32[3, 1]]: ...
    @property
    def index(self) -> int: ...
    @property
    def specie(self) -> int: ...

class SqsCallbackContextDouble:
    def __init__(self, *args, **kwargs) -> None: ...
    def stop(self) -> None: ...
    @property
    def statistics(self) -> SqsStatisticsDataDouble: ...

class SqsCallbackContextFloat:
    def __init__(self, *args, **kwargs) -> None: ...
    def stop(self) -> None: ...
    @property
    def statistics(self) -> SqsStatisticsDataFloat: ...

class SqsConfigurationDouble:
    chunk_size: int
    composition: list[Sublattice]
    iteration_mode: IterationMode
    iterations: int | None
    pair_weights: Incomplete
    prefactors: Incomplete
    shell_radii: list[list[float]]
    shell_weights: list[dict[int, float]]
    sublattice_mode: SublatticeMode
    target_objective: Incomplete
    thread_config: list[int]
    def __init__(self, *args, **kwargs) -> None: ...
    def bytes(self) -> bytes: ...
    @staticmethod
    def from_bytes(arg0: str) -> SqsConfigurationDouble: ...
    def structure(self) -> StructureDouble: ...

class SqsConfigurationFloat:
    chunk_size: int
    composition: list[Sublattice]
    iteration_mode: IterationMode
    iterations: int | None
    pair_weights: Incomplete
    prefactors: Incomplete
    shell_radii: list[list[float]]
    shell_weights: list[dict[int, float]]
    sublattice_mode: SublatticeMode
    target_objective: Incomplete
    thread_config: list[int]
    def __init__(self, *args, **kwargs) -> None: ...
    def bytes(self) -> bytes: ...
    @staticmethod
    def from_bytes(arg0: str) -> SqsConfigurationFloat: ...
    def structure(self) -> StructureFloat: ...

class SqsResultInteractDouble:
    def __init__(self, *args, **kwargs) -> None: ...
    def rank(self, base: int = ...) -> str: ...
    def shell_index(self, shell: int) -> int | None: ...
    def species_index(self, species: int) -> int | None: ...
    @overload
    def sro(self, shell: int, i: str, j: str) -> SroParameterDouble: ...
    @overload
    def sro(self, shell: int, i: int, j: int) -> SroParameterDouble: ...
    @overload
    def sro(self, shell: int) -> list[SroParameterDouble]: ...
    @overload
    def sro(self, i: int, j: int) -> list[SroParameterDouble]: ...
    @overload
    def sro(self, i: str, j: str) -> list[SroParameterDouble]: ...
    def structure(self) -> StructureDouble: ...
    @property
    def objective(self) -> float: ...

class SqsResultInteractFloat:
    def __init__(self, *args, **kwargs) -> None: ...
    def rank(self, base: int = ...) -> str: ...
    def shell_index(self, shell: int) -> int | None: ...
    def species_index(self, species: int) -> int | None: ...
    @overload
    def sro(self, shell: int, i: str, j: str) -> SroParameterFloat: ...
    @overload
    def sro(self, shell: int, i: int, j: int) -> SroParameterFloat: ...
    @overload
    def sro(self, shell: int) -> list[SroParameterFloat]: ...
    @overload
    def sro(self, i: int, j: int) -> list[SroParameterFloat]: ...
    @overload
    def sro(self, i: str, j: str) -> list[SroParameterFloat]: ...
    def structure(self) -> StructureFloat: ...
    @property
    def objective(self) -> float: ...

class SqsResultPackInteractDouble:
    def __init__(self, *args, **kwargs) -> None: ...
    def best(self) -> SqsResultInteractDouble: ...
    def bytes(self) -> bytes: ...
    @staticmethod
    def from_bytes(bytes: str) -> SqsResultPackInteractDouble: ...
    def num_objectives(self) -> int: ...
    def num_results(self) -> int: ...
    def __iter__(self) -> Iterator[tuple[float, list[SqsResultInteractDouble]]]: ...
    def __len__(self) -> int: ...
    @property
    def config(self) -> SqsConfigurationDouble: ...
    @property
    def statistics(self) -> SqsStatisticsDataDouble: ...

class SqsResultPackInteractFloat:
    def __init__(self, *args, **kwargs) -> None: ...
    def best(self) -> SqsResultInteractFloat: ...
    def bytes(self) -> bytes: ...
    @staticmethod
    def from_bytes(bytes: str) -> SqsResultPackInteractFloat: ...
    def num_objectives(self) -> int: ...
    def num_results(self) -> int: ...
    def __iter__(self) -> Iterator[tuple[float, list[SqsResultInteractFloat]]]: ...
    def __len__(self) -> int: ...
    @property
    def config(self) -> SqsConfigurationFloat: ...
    @property
    def statistics(self) -> SqsStatisticsDataFloat: ...

class SqsResultPackSplitDouble:
    def __init__(self, *args, **kwargs) -> None: ...
    def best(self) -> SqsResultSplitDouble: ...
    def bytes(self) -> bytes: ...
    @staticmethod
    def from_bytes(bytes: str) -> SqsResultPackSplitDouble: ...
    def num_objectives(self) -> int: ...
    def num_results(self) -> int: ...
    def __iter__(self) -> Iterator[tuple[float, list[SqsResultSplitDouble]]]: ...
    def __len__(self) -> int: ...
    @property
    def config(self) -> SqsConfigurationDouble: ...
    @property
    def statistics(self) -> SqsStatisticsDataDouble: ...

class SqsResultPackSplitFloat:
    def __init__(self, *args, **kwargs) -> None: ...
    def best(self) -> SqsResultSplitFloat: ...
    def bytes(self) -> bytes: ...
    @staticmethod
    def from_bytes(bytes: str) -> SqsResultPackSplitFloat: ...
    def num_objectives(self) -> int: ...
    def num_results(self) -> int: ...
    def __iter__(self) -> Iterator[tuple[float, list[SqsResultSplitFloat]]]: ...
    def __len__(self) -> int: ...
    @property
    def config(self) -> SqsConfigurationFloat: ...
    @property
    def statistics(self) -> SqsStatisticsDataFloat: ...

class SqsResultSplitDouble:
    def __init__(self, *args, **kwargs) -> None: ...
    def structure(self) -> StructureDouble: ...
    def sublattices(self) -> list[SqsResultInteractDouble]: ...
    @property
    def objective(self) -> float: ...

class SqsResultSplitFloat:
    def __init__(self, *args, **kwargs) -> None: ...
    def structure(self) -> StructureFloat: ...
    def sublattices(self) -> list[SqsResultInteractFloat]: ...
    @property
    def objective(self) -> float: ...

class SqsStatisticsDataDouble:
    def __init__(self, *args, **kwargs) -> None: ...
    @property
    def best_objective(self) -> float: ...
    @property
    def best_rank(self) -> int: ...
    @property
    def finished(self) -> int: ...
    @property
    def timings(self) -> dict[Timing, int]: ...
    @property
    def working(self) -> int: ...

class SqsStatisticsDataFloat:
    def __init__(self, *args, **kwargs) -> None: ...
    @property
    def best_objective(self) -> float: ...
    @property
    def best_rank(self) -> int: ...
    @property
    def finished(self) -> int: ...
    @property
    def timings(self) -> dict[Timing, int]: ...
    @property
    def working(self) -> int: ...

class SroParameterDouble:
    def __init__(self, *args, **kwargs) -> None: ...
    def __float__(self) -> float: ...
    @property
    def i(self) -> int: ...
    @property
    def j(self) -> int: ...
    @property
    def shell(self) -> int: ...
    @property
    def value(self) -> float: ...

class SroParameterFloat:
    def __init__(self, *args, **kwargs) -> None: ...
    def __float__(self) -> float: ...
    @property
    def i(self) -> int: ...
    @property
    def j(self) -> int: ...
    @property
    def shell(self) -> int: ...
    @property
    def value(self) -> float: ...

class StructureDouble:
    @overload
    def __init__(self, lattice: numpy.ndarray[numpy.float64[3, 3]], coords: numpy.ndarray[numpy.float64[m, 3]], species: list[int], pbc) -> None: ...
    @overload
    def __init__(self, lattice: numpy.ndarray[numpy.float64[3, 3]], coords: numpy.ndarray[numpy.float64[m, 3]], species: list[int]) -> None: ...
    def bytes(self) -> bytes: ...
    def dump(self, arg0: StructureFormat) -> str: ...
    @staticmethod
    def from_bytes(bytes: str) -> StructureDouble: ...
    @staticmethod
    def from_json(arg0: str, arg1: StructureFormat) -> StructureDouble: ...
    @staticmethod
    def from_poscar(string: str) -> StructureDouble: ...
    def shell_matrix(self, shell_radii: list[float], atol: float = ..., rtol: float = ...) -> numpy.ndarray[numpy.uint32[m, n]]: ...
    def supercell(self, sa: int, sb: int, sc: int) -> StructureDouble: ...
    def __eq__(self, arg0: StructureDouble) -> bool: ...
    def __iter__(self): ...
    def __len__(self) -> int: ...
    def __mul__(self, arg0: tuple) -> StructureDouble: ...
    @property
    def atoms(self) -> list[Atom]: ...
    @property
    def distance_matrix(self) -> numpy.ndarray[numpy.float64[m, n]]: ...
    @property
    def frac_coords(self) -> numpy.ndarray[numpy.float64[m, 3]]: ...
    @property
    def lattice(self) -> numpy.ndarray[numpy.float64[3, 3]]: ...
    @property
    def num_species(self) -> int: ...
    @property
    def sites(self): ...
    @property
    def species(self) -> list[int]: ...
    @property
    def symbols(self) -> list[str]: ...
    @property
    def uuid(self) -> object: ...

class StructureFloat:
    @overload
    def __init__(self, lattice: numpy.ndarray[numpy.float32[3, 3]], coords: numpy.ndarray[numpy.float32[m, 3]], species: list[int], pbc) -> None: ...
    @overload
    def __init__(self, lattice: numpy.ndarray[numpy.float32[3, 3]], coords: numpy.ndarray[numpy.float32[m, 3]], species: list[int]) -> None: ...
    def bytes(self) -> bytes: ...
    def dump(self, arg0: StructureFormat) -> str: ...
    @staticmethod
    def from_bytes(bytes: str) -> StructureFloat: ...
    @staticmethod
    def from_json(arg0: str, arg1: StructureFormat) -> StructureFloat: ...
    @staticmethod
    def from_poscar(string: str) -> StructureFloat: ...
    def shell_matrix(self, shell_radii: list[float], atol: float = ..., rtol: float = ...) -> numpy.ndarray[numpy.uint32[m, n]]: ...
    def supercell(self, sa: int, sb: int, sc: int) -> StructureFloat: ...
    def __eq__(self, arg0: StructureFloat) -> bool: ...
    def __iter__(self): ...
    def __len__(self) -> int: ...
    def __mul__(self, arg0: tuple) -> StructureFloat: ...
    @property
    def atoms(self) -> list[Atom]: ...
    @property
    def distance_matrix(self) -> numpy.ndarray[numpy.float32[m, n]]: ...
    @property
    def frac_coords(self) -> numpy.ndarray[numpy.float32[m, 3]]: ...
    @property
    def lattice(self) -> numpy.ndarray[numpy.float32[3, 3]]: ...
    @property
    def num_species(self) -> int: ...
    @property
    def sites(self): ...
    @property
    def species(self) -> list[int]: ...
    @property
    def symbols(self) -> list[str]: ...
    @property
    def uuid(self) -> object: ...

class StructureFormat:
    __members__: ClassVar[dict] = ...  # read-only
    __entries: ClassVar[dict] = ...
    cif: ClassVar[StructureFormat] = ...
    json_ase: ClassVar[StructureFormat] = ...
    json_pymatgen: ClassVar[StructureFormat] = ...
    json_sqsgen: ClassVar[StructureFormat] = ...
    poscar: ClassVar[StructureFormat] = ...
    def __init__(self, value: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class Sublattice:
    composition: dict[int, int]
    sites: Indices
    def __init__(self, arg0: Indices, arg1: dict[int, int]) -> None: ...

class SublatticeMode:
    __members__: ClassVar[dict] = ...  # read-only
    __entries: ClassVar[dict] = ...
    interact: ClassVar[SublatticeMode] = ...
    split: ClassVar[SublatticeMode] = ...
    def __init__(self, value: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class Timing:
    __members__: ClassVar[dict] = ...  # read-only
    __entries: ClassVar[dict] = ...
    chunk_setup: ClassVar[Timing] = ...
    comm: ClassVar[Timing] = ...
    loop: ClassVar[Timing] = ...
    total: ClassVar[Timing] = ...
    undefined: ClassVar[Timing] = ...
    def __init__(self, value: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: object) -> bool: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

def load_result_pack(data: str, prec: Prec = ...) -> SqsResultPackSplitFloat | SqsResultPackSplitDouble | SqsResultPackInteractFloat | SqsResultPackInteractDouble: ...
def optimize(*args, **kwargs): ...
def parse_config(*args, **kwargs): ...
