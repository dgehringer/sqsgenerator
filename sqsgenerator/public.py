import signal
import warnings
import numpy as np
import typing as T
from attrdict import AttrDict
from operator import attrgetter as attr
from sqsgenerator.io import read_settings_file
from sqsgenerator.settings import construct_settings, process_settings
from sqsgenerator.adapters import to_pymatgen_structure, to_ase_atoms, from_pymatgen_structure, from_ase_atoms
from sqsgenerator.core import log_levels, set_core_log_level, pair_sqs_iteration as pair_sqs_iteration_core, \
    SQSResult, symbols_from_z, Structure, make_supercell, IterationMode, pair_analysis, available_species

__all__ = [
    'IterationMode',
    'Structure',
    'SQSResult',
    'make_supercell',
    'process_settings',
    'make_result_document',
    'pair_sqs_iteration',
    'sqs_optimize',
    'from_pymatgen_structure',
    'from_ase_atoms',
    'pair_analysis',
    'available_species',
    'read_settings_file'
]

TimingDictionary = T.Dict[int, T.List[float]]
Settings = AttrDict
SQSResultCollection = T.Iterable[SQSResult]


def make_result_document(settings: Settings, sqs_results: T.Iterable[SQSResult],
                         timings: T.Optional[TimingDictionary] = None,
                         fields: T.Tuple[str, ...] = ('configuration',)) -> Settings:
    """
    Converts the ``sqsgenerator.core.SQSResults`` obtained from ``pair_sqs_results`` into a JSON/YAML serializable
    dictionary

    :param settings: the settings dictionary used to compute {sqs_results}
    :type settings: AttrDict
    :param sqs_results: the ``sqsgenerator.core.SQSResults`` calculated by ``pair_sqs_results``
    :type sqs_results: iterable of ``sqsgenerator.core.SQSResults``
    :param timings: a dictionary of thread timing information (default is ``None``)
    :type timings: Dict[int, float]
    :param fields: the fields to include in the document. Can be either *configuration*, *objective* and/or
        *parameters* (default is ``('configuration',)``)
    :type fields: Tuple[str, ...]
    :return: the JSON/YAML serializable document
    :rtype: AttrDict
    """
    allowed_fields = dict(
        configuration=lambda result: symbols_from_z(result.configuration),
        objective=attr('objective'),
        parameters=lambda result: result.parameters(settings.target_objective.shape)
    )

    def make_sqs_result_document(result):
        if len(fields) == 1:
            key = next(iter(fields))
            return allowed_fields[key](result)
        else:
            return {f: allowed_fields[f](result) for f in fields}

    result_document = dict(
        structure=settings.structure,
        configurations={r.rank: make_sqs_result_document(r) for r in sqs_results},
        which=settings.which
    )

    if timings is not None:
        result_document['timings'] = timings
    return Settings(result_document)


def extract_structures(results: Settings) -> T.Dict[int, Structure]:
    """
    Parses a dictionary of results and replaces the generated configuration with the actual structure

    :param results: the dict-like iteration results
    :type results:  AttrDict
    :return: a dictionary with ranks structure objects as values
    :rtype: Dict[int, :py:class:`sqsgenerator.public.Structure`]
    """
    structure: Structure = results.structure

    raw_data = results['configuration'] if 'configuration' in results else results

    def get_configuration(conf):
        return conf if not isinstance(conf, dict) else conf['configuration']

    structures = {
        rank:
            structure.with_species(get_configuration(conf), which=results.which)
        for rank, conf in raw_data.items()
    }
    return structures


def pair_sqs_iteration(settings: Settings, minimal: bool = True, similar: bool = False, log_level: str = 'warning') -> \
        T.Tuple[SQSResultCollection, TimingDictionary]:
    """
    Performs an SQS iteration using the {settings} configuration

    :param settings: the dict-like settings used for the iteration. Please refer to the
        `Input parameter <https://sqsgenerator.readthedocs.io/en/latest/input_parameters.html>`_ for further information
    :type settings: AttrDict
    :param minimal: if the result vector contains ``sqsgenerator.core.SQSResult`` objects with different objective
        values, select only those with minimal value of the objective function (default is ``True``)
    :type minimal: bool
    :param similar: in case the result vector contains more than one structure with minimal objective, include also
        degenerate solutions (default is ``False``)
    :type similar: bool
    :param log_level: the log level of the core extension. Valid values are "*trace*", "*debug*", "*info*", "*warning*"
        and "*error*". Please use "*trace*" when you encounter a bug in the core extension and report an issue
        (default is ``"warning"``)
    :type log_level: str
    :return: the minimal configuration and the corresponding Short-range-order parameters as well as timing information
    :rtype: Tuple[Iterable[:py:class:`sqsgenerator.public.SQSResult`], Dict[int, float]]
    """
    set_core_log_level(log_levels.get(log_level))

    iteration_settings = construct_settings(settings, False, structure=settings.structure[settings.which])

    interrupted = False

    def handle_sigint(signumber, *_):
        assert signumber == signal.SIGINT
        nonlocal interrupted
        interrupted = True

    original_handler = signal.getsignal(signal.SIGINT)
    signal.signal(signal.SIGINT, handle_sigint)
    sqs_results, timings = pair_sqs_iteration_core(iteration_settings)
    signal.signal(signal.SIGINT, original_handler)  # restore the old signal handler

    if interrupted:
        warnings.warn('SIGINT received: SQS results may be incomplete')
    best_result = min(sqs_results, key=attr('objective'))
    if minimal:
        sqs_results = list(filter(lambda r: np.isclose(r.objective, best_result.objective), sqs_results))

        if not similar:
            shape = settings.target_objective.shape
            sqs_results = list(
                filter(
                    lambda r: not np.allclose(r.parameters(shape), best_result.parameters(shape)),
                    sqs_results
                )
            )
            sqs_results.append(best_result)

    return sqs_results, timings


def expand_sqs_results(settings: Settings, sqs_results: T.Iterable[SQSResult],
                       timings: T.Optional[TimingDictionary] = None, fields: T.Tuple[str, ...] = ('configuration',),
                       inplace: bool = False) -> Settings:
    """
    Serializes a list of :py:class:`sqsgenerator.public.SQSResult` into a JSON/YAML serializable dictionary

    :param settings: the settings used to compute the {sqs_results}
    :type settings: AttrDict
    :param sqs_results: a iterable (list) of :py:class:`sqsgenerator.public.SQSResult`
    :type sqs_results: Iterable[:py:class:`sqsgenerator.public.SQSResult`]
    :param timings: a dict like information about the performance of the core routines. Keys refer to thread numbers.
        the values represent the average time the thread needed to analyse one configuration in **µs** (default is ``None``)
    :type timings: Dict[int, float]
    :param fields: a tuple of fields to include. Allowed fields are "*configuration*", "*objective*", and
        "*parameters*" (default is ``('configuration',)``)
    :type fields: Tuple[str, ...]
    :param inplace: update the the input ``settings`` document instead of creating a new one (default is ``False``)

    """
    dump_include = list(fields)
    if 'configuration' not in dump_include:
        dump_include += ['configuration']
    result_document = make_result_document(settings, sqs_results, fields=tuple(dump_include), timings=timings)

    if inplace:
        settings.update(result_document)
        keys_to_remove = {'file_name', 'input_format', 'composition', 'iterations', 'max_output_configurations',
                          'mode', 'threads_per_rank', 'is_sublattice'}
        final_document = {k: v for k, v in settings.items() if k not in keys_to_remove}
        if 'sublattice' in final_document:
            final_document.update(final_document['sublattice'])
            del final_document['sublattice']
    else:
        final_document = result_document

    return Settings(final_document)


def merge(a: dict, b: T.Optional[dict] = None, **kwargs) -> dict:
    if b is None:
        b = {}
    return {**a, **b, **kwargs}


def sqs_optimize(settings: T.Union[Settings, T.Dict], process: bool = True, minimal: bool = True,
                 similar: bool = False, log_level: str = 'warning',
                 fields: T.Tuple[str, ...] = ('configuration', 'parameters', 'objective'),
                 make_structures: bool = False, structure_format: str = 'default') \
        -> T.Tuple[T.Dict[int, T.Dict[str, T.Any]], T.Dict[int, T.Union[float, T.List[float]]]]:
    """
    This function allows to simply generate SQS structures

    Performs a SQS optimization loop. This function is meant for using sqsgenerator through Python. Prefer this function
    over :py:func:`sqsgenerator.public.pair_sqs_iteration`. It combines the functionalities of several low-level utility
    function.

        1. Generate default values for ``settings`` (:py:func:`sqsgenerator.public.process_settings`)
        2. Execute the actual SQS optimization loop (:py:func:`sqsgenerator.public.pair_sqs_iteration`)
        3. Process, convert the results (:py:func:`sqsgenerator.public.make_result_document`)
        4. Build the structures from the optimization results (:py:func:`sqsgenerator.public.extract_structures`)

    An example output might look like the following:

        .. code-block:: python

            {
                654984: {
                    'configuration': ['Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W']
                    'objective': 0.0,
                    # only present if make_structures=True
                    # Atoms object if structure_format='ase'
                    'structure': Atoms(symbols='ReWReWReWReWReWReWReWReW', pbc=True, cell=[6.33, 6.33, 6.33])
                }
            }

    :param settings: the settings used for the SQS optimization
    :type settings: AttrDict or Dict
    :param process: process the input {settings} dictionary (default is ``True``)
    :type process: bool
    :param minimal: Include only configurations with minimum objective function in the results (default is ``True``)
    :type minimal: bool
    :param similar: If the minimum objective is degenerate include also results with same parameters but
        different configuration (default is ``False``)
    :type similar: bool
    :param log_level: set's the log level for the core C++ extension. Possible fields are "*trace*", "*debug*",
        "*info*", "*warning*" and "*error*" (default is ``'warning'``)
    :type log_level: str
    :param fields: output fields included in the result document. Possible fields are "*configuration*", "*parameters*",
        "*objective*" and "*parameters*" (default is ``('configuration',)``)
    :type fields: Tuple[str, ...]
    :param make_structures: build structure objects from the optimization results (default is ``False``
    :type make_structures: bool
    :param structure_format: if {make_structures} was set to ``True`` it specifies the format of the build structures
        (default is ``'default'``)

            - "*default*": :py:class:`sqsgenerator.public.Structure`
            - "*pymatgen*" :py:class:`pymatgen.core.Structure`
            - "*ase*": :py:class:`ase.atoms.Atoms`

    :type structure_format: str
    :return: a dictionary with the specified fields as well as timing information. The keys of the result dictionary are
        the permutation ranks of the generated configuration.
    :rtype: T.Tuple[T.Dict[int, T.Dict[str, T.Any]], T.Dict[int, T.Union[float, T.List[float]]]]

    """
    settings = settings if isinstance(settings, Settings) else AttrDict(settings)
    settings = process_settings(settings) if process else settings

    results, timings = pair_sqs_iteration(settings, minimal=minimal, similar=similar, log_level=log_level)

    result_document = make_result_document(settings, results, timings=timings, fields=fields).get('configurations')
    if make_structures:
        structure_document = extract_structures(result_document)
        converters = dict(default=lambda _: _, ase=to_ase_atoms, pymatgen=to_pymatgen_structure)
        converter = converters.get(structure_format)
        structure_document = {k: converter(v) for k, v in structure_document.items()}

    if make_structures:
        result_document = {rank: merge(result, structure=structure_document[rank]) for rank, result in
                           result_document.items()}

    return result_document, timings
