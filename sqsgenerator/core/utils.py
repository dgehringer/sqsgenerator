
import typing as T


def transpose(iterable: T.Iterable) -> zip:
    return zip(*iterable)


def merge(a: dict, b: T.Optional[T.Dict[T.Any, T.Any]] = None, **kwargs) -> T.Dict[T.Any, T.Any]:
    """
    Merges a dictionary {a} and {b} as well as {kwargs} into a new one

    :param a: the first dictionary
    :type a: Dict
    :param b: the second dictionary
    :type b: Dict
    :return: a merged dictionary
    :rtype: Dict
    """

    if b is None:
        b = {}
    return {**a, **b, **kwargs}


def chunks_of(it: T.Iterable[T.Any], n: int = 1) -> T.Iterable[T.List[T.Any]]:
    """
    Partitions an iterator {it} into chunks of {n}. If n  is not a divisor of the length of it the last chunk will
    be smaller

    :param it: the iterator to chunk
    :type it: Iterable[Any]
    :param n: window size {n} in which the iterator will be partitioned
    :type n: int
    :return: an iterable of length {n}
    :rtype: Iterable[List[Any]]
    """

    current_chunk = []
    for element in it:
        current_chunk.append(element)
        if len(current_chunk) == n:
            yield current_chunk
            current_chunk = []
    if current_chunk:
        yield current_chunk
