import enum
import attrdict
import functools
import typing as T
from .exceptions import BadSettings
from sqsgenerator.core import get_function_logger


class Default(enum.Enum):
    NoDefault = 0


def parameter(name: str, default: T.Optional[T.Any] = Default.NoDefault, required: T.Union[T.Callable, bool]=True, key: T.Union[T.Callable, str] = None, registry: T.Optional[dict] = None):
    if key is None: key = name
    get_required = lambda *_: required if isinstance(required, bool) else required
    get_key = lambda *_: key if isinstance(key, str) else key
    get_default = (lambda *_: default) if not callable(default) else default

    have_default = default != Default.NoDefault

    def _decorator(f: T.Callable):
        @functools.wraps(f)
        def _wrapped(settings: attrdict.AttrDict):
            is_required = get_required(settings)
            k = get_key(settings)
            nonlocal name
            if k not in settings:
                if is_required:
                    if not have_default:
                        raise BadSettings(f'Required parameter "{name}" was not found', parameter=name)
                    else:
                        # a default is needed but found
                        df = get_default(settings)
                        get_function_logger(f).info(f'Parameter "{name}" was not found defaulting to: "{df}"')
                        return df
            else:
                # we catch the exception here and raise it again, to inject the parameter information automatically
                try: processed_value = f(settings)
                except BadSettings as exc:
                    exc.parameter = name # set the parameter info and forward the exception
                    raise
                else: return processed_value

        # register the parameters at the parameter_registry
        if registry is not None: registry[name] = _wrapped
        return _wrapped

    return _decorator


def if_(condition):
    def then_(th_val):
        def else_(el_val):
            def stmt_(settings): return th_val if condition(settings) else el_val
            return stmt_
        return else_
    return then_


def isa(o: T.Tuple[type]):
    """
    Constructs a type-checking predicate. Represents isinstance(x, o)
    :param o: the types to checked
    :type o: type
    :return: the predicate
    :rtype: Callable
    """
    return lambda x: isinstance(x, o)


def identity():
    return lambda _: _