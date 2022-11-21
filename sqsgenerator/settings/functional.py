import enum
import functools
import typing as T
from sqsgenerator.core import get_function_logger
from sqsgenerator.fallback.attrdict import AttrDict
from sqsgenerator.settings.exceptions import BadSettings
from sqsgenerator.compat import FeatureNotAvailableException


class Default(enum.Enum):
    """
    Dummy Enum to represent "no default" value. ``None`` is not possible, since it is a legit default value, we
    use  ``Default.NoDfault``to represent no default value
    """
    NoDefault = 0


def parameter(name: str, default: T.Optional[T.Any] = Default.NoDefault, required: T.Union[T.Callable, bool] = True,
              key: T.Union[T.Callable, str] = None, registry: T.Optional[dict] = None):
    if key is None:
        key = name

    def get_required(settings):
        return required if isinstance(required, bool) else required(settings)

    def get_key(settings):
        return key if isinstance(key, str) else key(settings)

    get_default = (lambda *_: default) if not callable(default) else default

    have_default = default != Default.NoDefault

    def _decorator(f: T.Callable):
        @functools.wraps(f)
        def _wrapped(settings: AttrDict):
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
                try:
                    processed_value = f(settings)
                except BadSettings as exc:
                    exc.parameter = name  # set the parameter info and forward the exception
                    raise
                except FeatureNotAvailableException as exc:

                    raise BadSettings(
                        message='Missing at least one of the following features: ' + ', '.join(
                            f'"{f.value}"' for f in exc.features),
                        parameter=name
                    )
                else:
                    return processed_value

        # register the parameters at the parameter_registry
        if registry is not None:
            registry[name] = _wrapped
        return _wrapped

    return _decorator


def if_(condition):
    def then_(th_val):
        def else_(el_val):
            def stmt_(settings):
                return th_val if condition(settings) else el_val

            return stmt_

        return else_

    return then_


def isa(o: T.Union[type]):
    """
    Constructs a type-checking predicate. Represents isinstance(x, o)
    :param o: the types to checked
    :type o: type
    :return: the predicate
    :rtype: Callable
    """
    return lambda x: isinstance(x, o)


def star(f: T.Callable) -> T.Callable:
    return lambda x: f(*x)


def const(o: T.Any) -> T.Callable:
    return lambda *_: o


def identity() -> T.Callable:
    return lambda _: _  # pragma: no cover
