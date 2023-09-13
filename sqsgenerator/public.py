"""
This module forwards public imports from sqsgenerator.core and defines functions which are designed as user-functions
"""
import functools
import signal
import warnings
import itertools
import numpy as np
import typing as T
from sqsgenerator.fallback.attrdict import AttrDict
from sqsgenerator.settings.readers import read_structure
from operator import attrgetter as attr, itemgetter as item
from sqsgenerator.io import read_settings_file, export_structures
from sqsgenerator.settings import construct_settings, process_settings, defaults, build_structure as build_structure_
from sqsgenerator.adapters import to_pymatgen_structure, to_ase_atoms, to_pyiron_atoms, from_pymatgen_structure, \
    from_ase_atoms, from_pyiron_atoms
from sqsgenerator.core import log_levels, set_core_log_level, pair_sqs_iteration as pair_sqs_iteration_core, \
    SQSResult, symbols_from_z, Structure, make_supercell, IterationMode, pair_analysis, available_species, make_rank, \
    rank_structure, total_permutations, merge, SQSCallback, SQSCoreCallback, IterationSettings

TimingDictionary = T.Dict[int, T.List[float]]
Settings = AttrDict
SQSResultCollection = T.Iterable[SQSResult]
SQSCallbacks = T.Iterable[SQSCallback]
OptimizationResult = T.Tuple[T.Dict[int, T.Dict[str, T.Any]], T.Dict[int, T.Union[float, T.List[float]]]]

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
    'read_settings_file',
    'export_structures',
    'make_rank',
    'rank_structure',
    'total_permutations',
    'extract_structures',
    'expand_sqs_results',
    'sqs_analyse',
    'OptimizationResult',
    'Settings'
]


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


def extract_structures(results: Settings, base_structure: T.Optional[Structure] = None) -> T.Dict[int, Structure]:
    """
    Parses a dictionary of results and replaces the generated configuration with the actual structure

    :param results: the dict-like iteration results
    :type results:  AttrDict
    :param base_structure: the structure to which the individual configuration are applied to. If ``None`` it tries
        to extract it from the results document by calling the "structure" attribute (default is ``None``)
    :return: a dictionary with ranks structure objects as values
    :rtype: Dict[int, :py:class:`Structure`]
    """

    structure = results.structure if base_structure is None else base_structure
    which = results.which if base_structure is None else tuple(range(len(structure)))

    raw_data = results['configurations'] if 'configurations' in results else results

    def get_configuration(conf):
        return conf if not isinstance(conf, dict) else conf['configuration']

    structures = {
        rank:
            structure.with_species(get_configuration(conf), which=which)
        for rank, conf in raw_data.items()
    }
    return structures


def inject_structure(settings: Settings, f: SQSCallback) -> SQSCoreCallback:
    base_structure: Structure = settings.structure[settings.which]

    def callback(iteration: int, result: SQSResult, rank_id: int, thread_id: int) -> T.Optional[bool]:
        return f(iteration, base_structure.with_species(symbols_from_z(result.configuration)), result, rank_id,
                 thread_id)

    return callback


def pair_sqs_iteration(settings: Settings, minimal: bool = True, similar: bool = False, log_level: str = 'warning',
                       pass_structure: bool = False) -> \
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
    :param pass_structure: construct a :py:class:`Structure` object and pass it to the callback. If *True* the callback
        exhibits a signature of ``cb(iteration: int, structure: :py:class:`Structure`, parameters :py:class:`SQSResult`, rank_id: int, thread_id: int)``.
        If set to *False* the callbacks signature is ``cb(iteration: int, parameters :py:class:`SQSResult`, rank_id: int, thread_id: int)``
    :type pass_structure: bool
    :rtype: Tuple[Iterable[:py:class:`SQSResult`], Dict[int, float]]
    """
    set_core_log_level(log_levels.get(log_level))

    inject_structure_for_settings = functools.partial(inject_structure, settings)

    if pass_structure:
        settings.callbacks = {cb_name: list(map(inject_structure_for_settings, cbs)) for cb_name, cbs in
                              settings.callbacks.items()}

    iteration_settings = construct_settings(settings, False, structure=settings.structure[settings.which])

    interrupted = False

    def handle_sigint(signum, *_):
        assert signum == signal.SIGINT
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
    Serializes a list of :py:class:`SQSResult` into a JSON/YAML serializable dictionary

    :param settings: the settings used to compute the {sqs_results}
    :type settings: AttrDict
    :param sqs_results: a iterable (list) of :py:class:`SQSResult`
    :type sqs_results: Iterable[:py:class:`SQSResult`]
    :param timings: a dict like information about the performance of the core routines. Keys refer to thread numbers.
        The values represent the average time the thread needed to analyse one configuration in **µs**
        (default is ``None``)
    :type timings: Dict[int, float]
    :param fields: a tuple of fields to include. Allowed fields are "*configuration*", "*objective*", and
        "*parameters*" (default is ``('configuration',)``)
    :type fields: Tuple[str, ...]
    :param inplace: update the input ``settings`` document instead of creating a new one (default is ``False``)

    """
    dump_include = list(fields)
    if 'configuration' not in dump_include:
        dump_include += ['configuration']
    result_document = make_result_document(settings, sqs_results, fields=tuple(dump_include), timings=timings)

    if inplace:
        settings = settings.copy()
        settings.update(result_document)
        keys_to_remove = {'file_name', 'input_format', 'composition', 'iterations', 'max_output_configurations',
                          'mode', 'threads_per_rank'}
        final_document = {k: v for k, v in settings.items() if k not in keys_to_remove}
    else:
        final_document = result_document

    return Settings(final_document)


def sqs_optimize(settings: T.Union[Settings, T.Dict], process: bool = True, minimal: bool = True,
                 similar: bool = False, log_level: str = 'warning',
                 fields: T.Tuple[str, ...] = ('configuration', 'parameters', 'objective'),
                 make_structures: bool = False, structure_format: str = 'default', pass_structure: bool = False) \
        -> OptimizationResult:
    """
    This function allows to simply generate SQS structures

    Performs a SQS optimization loop. This function is meant for using sqsgenerator through Python. Prefer this function
    over :py:func:`pair_sqs_iteration`. It combines the functionalities of several low-level utility
    function.

        1. Generate default values for {settings} (:py:func:`process_settings`)
        2. Execute the actual SQS optimization loop (:py:func:`pair_sqs_iteration`)
        3. Process, convert the results (:py:func:`make_result_document`)
        4. Build the structures from the optimization results (:py:func:`extract_structures`)

    An example output might look like the following:

        .. code-block:: python

            {
                654984: {
                    'configuration': ['Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W']
                    'objective': 0.0,
                    # only present if make_structures=True
                    # Atoms object if structure_format='ase'
                    'structure': Atoms(symbols='ReWReWReWReWReWReWReW', pbc=True, cell=[6.33, 6.33, 6.33])
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

            - "*default*": :py:class:`Structure`
            - "*pymatgen*" :py:class:`pymatgen.core.Structure`
            - "*ase*": :py:class:`ase.atoms.Atoms`
            - "*pyiron*": :py:class:`pyiron_atomistics.atomistics.structure.Atoms`

    :type structure_format: str
    :return: a dictionary with the specified fields as well as timing information. The keys of the result dictionary are
        the permutation ranks of the generated configuration.
        :param pass_structure: construct a :py:class:`Structure` object and pass it to the callback. If *True* the callback
        exhibits a signature of ``cb(iteration: int, structure: :py:class:`Structure`, parameters :py:class:`SQSResult`, rank_id: int, thread_id: int)``.
        If set to *False* the callbacks signature is ``cb(iteration: int, parameters :py:class:`SQSResult`, rank_id: int, thread_id: int)``
    :type pass_structure: bool
    :rtype: Tuple[Dict[int, Dict[str, Any]], Dict[int, Union[float, List[float]]]]

    """
    settings = settings if isinstance(settings, Settings) else AttrDict(settings)
    settings = process_settings(settings) if process else settings

    results, timings = pair_sqs_iteration(settings, minimal=minimal, similar=similar, log_level=log_level, pass_structure=pass_structure)

    result_document = expand_sqs_results(settings, results, timings=timings, fields=fields)
    if make_structures:
        structure_document = extract_structures(result_document)
        converter = dict(default=lambda _: _, ase=to_ase_atoms, pymatgen=to_pymatgen_structure,
                         pyiron=to_pyiron_atoms).get(structure_format)
        structure_document = {k: converter(v) for k, v in structure_document.items()}

        result_document = result_document.get('configurations')
        result_document = {rank: merge(result, structure=structure_document[rank]) for rank, result in
                           result_document.items()}
    else:
        result_document = result_document.get('configurations')

    return result_document, timings


def sqs_analyse(structures: T.Iterable[Structure], settings: T.Optional[T.Union[Settings, T.Dict]] = None,
                process: bool = True, fields: T.Tuple[str, ...] = ('configuration', 'parameters', 'objective'),
                structure_format: str = 'default', append_structures: bool = False) -> T.Dict[int, T.Dict[str, T.Any]]:
    """
    Uses the given settings {settings} and an iterable of :py:func:`Structure` and compute the short-range-order
    parameters, objective function. By default the {fields} = ('configuration', 'parameters', 'objective') are
    included.

    :param structures: an iterable of structures to analyse
    :type structures: Iterable[Union[Structure, :py:class`ase.atoms.Atoms`, :py:class:`pymatgen.core.Structure`]]
    :param settings: the settings used for the SQS optimization
    :type settings: AttrDict or Dict
    :param process: process the input {settings} dictionary (default is ``True``)
    :type process: bool
    :param fields: output fields included in the result document. Possible fields are "*configuration*", "*parameters*",
        "*objective*" and "*parameters*" (default is ``('configuration', 'parameters', 'objective')``)
    :type fields: Tuple[str, ...]
    :param structure_format: the input format of the items in {structures}
        (default is ``'default'``)

            - "*default*": :py:class:`Structure`
            - "*pymatgen*" :py:class:`pymatgen.core.Structure`
            - "*ase*": :py:class:`ase.atoms.Atoms`
            - "*pyiron*": :py:class:`pyiron_atomistics.atomistics.structure.Atoms`

    :type structure_format: str
    :param append_structures: append the initial {structures} to the analysed results (default is ``False``)
    :type append_structures: bool
    :return: a dictionary with the specified fields as well as timing information. The keys of the result dictionary are
        the permutation ranks of the {structures}.
    :rtype: Dict[int, Dict[str, Any]]
    """

    converter = dict(default=lambda _: _, ase=from_ase_atoms, pymatgen=from_pymatgen_structure,
                     pyiron=from_pyiron_atoms).get(structure_format)

    # convert the structures to Structure object and extract the sublattice if needed
    structures = map(lambda st: converter(st), structures)
    first_structure = next(structures, None)

    if first_structure is None:
        raise ValueError('The structure input iterable contains no structure')

    if settings is None:
        # construct default settings
        settings = AttrDict(structure=first_structure)
        settings.update(which=defaults.which(settings))
        settings = process_settings(settings)  # we ignore {process} flag as we have to compute default settings
    else:
        # if the settings object contains a structure object we rise a warning that we will overwrite it
        if 'structure' in settings:
            warnings.warn('Your settings for "sqs_analyse" contain a "structure" key. I will ignore it!'
                          ' Pass structures using the {structures} parameter!')
            del settings['structure']

        if 'composition' in settings:
            warnings.warn('You cannot specify a composition when analysing a SQS structure')

        # make sure which is in the settings object
        settings['which'] = settings['which'] if 'which' in settings else defaults.which(
            AttrDict(structure=first_structure))
        first_structure = first_structure[settings['which']]
        # in any case we need default composition dictionary
        settings['composition'] = defaults.composition(AttrDict(structure=first_structure, which=settings['which']))

    settings = AttrDict(settings)
    # we are sure the which and structure is there, hence we can set our value for first_structure
    slicer = item(settings.which)
    structures = map(slicer, structures)
    settings['structure'] = first_structure
    settings = process_settings(settings) if process else settings
    analyse_settings = AttrDict(**settings)

    # we have consumed the first element of the {structure} iterator we again assemble it
    # we create a copy of the iterator
    structures, structures_copy = itertools.tee(itertools.chain((first_structure,), structures))

    analysed = {
        rank_structure(st): pair_analysis(construct_settings(analyse_settings, False, structure=st))
        for st in structures
    }
    document = expand_sqs_results(analyse_settings, list(analysed.values()), fields=fields).get('configurations')

    if append_structures:
        for structure in structures_copy:
            rank = rank_structure(structure)
            assert rank in document
            document[rank]['structure'] = structure

    return document


def build_structure(settings: Settings) -> Structure:
    structure = read_structure(settings)
    return build_structure_(settings.get('compositions'), structure)
