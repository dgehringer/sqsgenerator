
import enum

import operator
import attrdict
import functools
import typing as T

attr = operator.attrgetter
item = operator.itemgetter


class Default(enum.Enum):
    NoDefault = 0


def transpose(iterable: T.Iterable): return zip(*iterable)


def if_(condition):
    def then_(th_val):
        def else_(el_val):
            def stmt_(settings): return th_val if condition(settings) else el_val
            return stmt_
        return else_
    return then_


def try_(f: T.Callable, *args, exc_type: T.Type = Exception, raise_exc: bool=True, return_success: bool =False, msg: T.Optional[str]=None, log_exc_info: bool=False, **kwargs) -> T.Any:
    try:
        result = f(*args, **kwargs)
        success = True
    except exc_type as e:
        if msg: get_function_logger(f).exception(f'Failed to {msg}', exc_info=e if log_exc_info else None)
        if raise_exc: raise
        result = None
        success = False
    return (success, result) if return_success else result
