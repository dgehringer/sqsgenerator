import functools
import numpy as np
import typing as T
from attrdict import AttrDict
from operator import attrgetter as attr
from sqsgenerator.settings.readers import read_structure
from sqsgenerator.settings import construct_settings, process_settings, build_structure
from sqsgenerator.core import log_levels, set_core_log_level, pair_sqs_iteration as pair_sqs_iteration_core, \
    SQSResult, symbols_from_z, Structure, pair_analysis, available_species, make_supercell, IterationMode


__all__ = [
    'IterationMode',
    'Structure',
    'SQSResult',
    'make_supercell',
    'process_settings',
    'make_result_document',
    'pair_sqs_iteration'
]

TimingDictionary = T.Dict[int, T.List[float]]
Settings = AttrDict
SQSResultCollection = T.Iterable[SQSResult]


def make_result_document(settings: Settings, sqs_results: T.Iterable[SQSResult],
                         timings: T.Optional[TimingDictionary] = None,
                         fields: T.Iterable[str] = ('configuration',)) -> Settings:
    """
    Converts the ``sqsgenerator.core.SQSResults`` obtained from ``pair_sqs_results`` into a JSON/YAML serializable
    dictionary

    :param settings: the settings dictionary used to compute {sqs_results}
    :type settings: AttrDict
    :param sqs_results: the ``sqsgenerator.core.SQSResults`` calculated by ``pair_sqs_results``
    :type sqs_results: iterable of ``sqsgenerator.core.SQSResults``
    :param timings: a dictionary of thread timing information (default is ``None``)
    :type timings: dict with int as keys and float as values
    :param fields: the fields to include in the document. Can be either *configuration*, *objective* and/or
        *parameters* (default is ``('configuration',)``)
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
    :rtype: dict with ``int`` as keys and ``Structure`` as values
    """
    structure: Structure = results.structure

    def get_configuration(conf):
        return conf if not isinstance(conf, dict) else conf['configuration']

    structures = {
        rank:
            structure.with_species(get_configuration(conf), which=results.which)
        for rank, conf in results['configurations'].items()
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
    :rtype: Tuple[Iterable[:py:class:`sqsgenerator.public.SQSResult`], Dict[``int``, ``float``]]
    """
    iteration_settings = construct_settings(settings, False, structure=settings.structure[settings.which])
    set_core_log_level(log_levels.get(log_level))
    sqs_results, timings = pair_sqs_iteration_core(iteration_settings)

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
                       timings: T.Optional[TimingDictionary] = None, include=('configuration',),
                       inplace: bool = False) -> Settings:
    """
    Serializes a list of :py:class:`sqsgenerator.public.SQSResult` into a JSON/YAML serializable string

    :param settings: the settings used to compute the {sqs_results}
    :type settings: AttrDict
    :param sqs_results:
    """
    dump_include = list(include)
    if 'configuration' not in dump_include:
        dump_include += ['configuration']
    result_document = make_result_document(settings, sqs_results, fields=dump_include, timings=timings)

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
