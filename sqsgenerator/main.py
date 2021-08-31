
import functools
import numpy as np
import typing as T
from attrdict import AttrDict
from operator import attrgetter as attr
from sqsgenerator.settings import construct_settings
from sqsgenerator.settings.readers import read_structure
from sqsgenerator.core import log_levels, set_core_log_level, pair_sqs_iteration as pair_sqs_iteration_core, SQSResult, symbols_from_z, Structure, pair_analysis, available_species

TimingDictionary = T.Dict[int, T.List[float]]
Settings = AttrDict
SQSResultCollection = T.Iterable[SQSResult]


def make_result_document(settings, sqs_results, timings=None, fields=('configuration',)) -> Settings:
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


def extract_structures(results: Settings):
    structure: Structure = results.structure

    def get_configuration(conf):
        return conf if not isinstance(conf, dict) else conf['configuration']
    structures = {
        rank:
        structure.with_species(get_configuration(conf), which=results.which)
        for rank, conf in results['configurations'].items()
    }
    return structures


def pair_sqs_iteration(settings: Settings, minimal: bool=True, similar: bool=False, log_level: str='warning') -> T.Tuple[SQSResultCollection, TimingDictionary]:
    structure = settings.structure.slice_with_species(settings.composition, settings.which)
    iteration_settings = construct_settings(settings, False, structure=structure)
    set_core_log_level(log_levels.get(log_level))
    sqs_results, timings = pair_sqs_iteration_core(iteration_settings)

    best_result = min(sqs_results, key=attr('objective'))
    if minimal:
        sqs_results = list(filter(lambda r: np.isclose(r.objective, best_result.objective), sqs_results))

        if not similar:
            shape = settings.target_objective.shape
            sqs_results = list(filter(lambda r: not np.allclose(r.parameters(shape), best_result.parameters(shape)), sqs_results))
            sqs_results.append(best_result)

    return sqs_results, timings


def expand_sqs_results(settings, sqs_results, timings=None, include=('configuration',), inplace=False):
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