import asyncio
import typing as T
from math import isclose
from functools import wraps
from operator import itemgetter as item
from sqsgenerator.core import chunks_of, merge, transpose
from sqsgenerator.public import Settings, sqs_optimize, Structure, OptimizationResult


@wraps(sqs_optimize)
async def sqs_optimize_async(process: bool = True, minimal: bool = True, similar: bool = False,
                             log_level: str = 'warning',
                             fields: T.Tuple[str, ...] = ('configuration', 'parameters', 'objective'),
                             make_structures: bool = False, structure_format: str = 'default', **kwargs):
    """
    async wrapper function around :py:func:`sqs_optimize`. The keyword arguments will be gathered and
    passed as settings to :py:func:`sqs_optimize`

    """
    settings = Settings(kwargs)

    return sqs_optimize(settings, process=process, minimal=minimal, similar=similar, log_level=log_level, fields=fields,
                        make_structures=make_structures, structure_format=structure_format)


async def sqsgen_optimize_multiple_async(it: T.Iterable[T.Dict[str, T.Any]],
                                         reduce_results: T.Callable[[T.Any, T.Any], T.Any],
                                         result_factory: T.Callable[[], T.Any],
                                         chunk_size: int = 1,
                                         loop: T.Optional[asyncio.AbstractEventLoop] = None) -> T.Any:
    """
    Performs sqs_optimization loops asynchronously

    :param it: input generator producing input dictionaries which will be passed on to :py:func:`sqs_optimize_async`
    :type it: Iterable[Dict[str, Any]]
    :param reduce_results: callable to reduce the results obtained from a sqs_optimize run
    :type reduce_results: Callable[[Any, Any], Any]
    :param result_factory: creates an *empty* result object
    :type result_factory: Callable[[], Any]
    :param chunk_size: optionally split the parameter (default is 1)
    :type chunk_size: int
    :param loop: optional (default is None)
    :return: the reduced results
    """
    if not loop:
        loop = asyncio.get_event_loop()
    chunks = chunks_of(it, chunk_size) if chunk_size > 1 else (it,)
    master_result = result_factory()
    for chunk in chunks:
        chunk_result = result_factory()
        done, pending = {}, {loop.create_task(sqs_optimize_async(**merge(kwargs, make_structures=True, **kwargs))) for
                             kwargs in chunk}
        while pending:
            done, pending = await asyncio.wait(pending, return_when=asyncio.FIRST_COMPLETED)
            for finished in done:
                chunk_result = reduce_results(chunk_result, finished.result())
        master_result = reduce_results(master_result, chunk_result)
    return master_result


def minimum_objective_and_structures(results: T.Tuple[float, T.List[Structure]], new_results: OptimizationResult) -> \
T.Tuple[float, T.List[Structure]]:
    best_objective, structures = results
    if isinstance(next(iter(new_results)), dict):
        new_results, *_ = new_results
        new_objective, new_structures = transpose(map(item('objective', 'structure'), new_results.values()))
        new_objective = min(new_objective)
    else:
        new_objective, new_structures = new_results

    if isclose(best_objective, new_objective):
        return min(best_objective, new_objective), structures + new_structures
    elif best_objective < new_objective:
        return results
    else:
        return new_objective, new_structures


@wraps(sqsgen_optimize_multiple_async)
def sqsgen_optimize_multiple(it: T.Iterable[T.Dict[str, T.Any]],
                             reduce_results: T.Callable[[T.Any, T.Any], T.Any],
                             result_factory: T.Callable[[], T.Any],
                             chunk_size: int = 1) -> T.Any:
    return asyncio.run(sqsgen_optimize_multiple_async(it, reduce_results, result_factory, chunk_size=chunk_size))


def sqsgen_minimize_multiple(it: T.Iterable[T.Dict[str, T.Any]], chunk_size: int = 1):
    return sqsgen_optimize_multiple(it, minimum_objective_and_structures, lambda: (float('inf'), []), chunk_size=chunk_size)
