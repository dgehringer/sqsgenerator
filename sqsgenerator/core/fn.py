"""
This module provides basic higher order functions, to provide basic functional programming by using function composition
Forwards the most important function from functools too. Can be used as a drop-in replacement for functools.
Allows for idiomatic function composition using the `Composable` class

The "left associative" function composition is implemented with the "@" operator (`__rmatmul__`)
Reason "@" comes visually closest to ° from what is available through Pythons magic functions
The "right associative" function composition is implemented with the "@" operator (`__rmatmul__`)

Bitwise shifts are overloaded to feed the "pipes"

Example "left associative":
    ```
    c_ = Composable
    c_(f) @ g  = circ(f, g) = f ° g = f(g(x))
    c_(f) @ g @ h j = f ° g  ° h  ° j = f(g(h(j(x))))
    ```

The right associative operator is more "intuitive" since the order resembles also the calling order

Example "right associative" - pipe:
    ```
    c_ = Composable
    c_(f) | g  = circ(f, g) = g ° f = f(g(x))
    c_(f) | g | h | j = j ° h  ° g  ° f = f(g(h(j(x))))
    ```
"""

__author__ = "Dominik Gehringer"
__email__ = "dgehringer@protonmail.com"
__license__ = "MIT"
__date__ = (2021, 3)


import enum
import attrdict
import operator
import itertools
import functools
import typing as T
import collections.abc
from sqsgenerator.core import get_function_logger, BadSettings
# Some typing shortcuts to provide concise typing
Predicate = T.Callable[[T.Any], bool]
Transform = T.Callable[[T.Any], T.Any]
Aggregator = T.Callable[[T.Any, T.Any], T.Any]
Action = T.Callable[[T.Any], T.NoReturn]
DefaultFunction = Transform
Iterable = collections.abc.Iterable

attr = operator.attrgetter
item = operator.itemgetter
method = operator.methodcaller
partial = functools.partial


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
            if k not in settings:
                nonlocal name
                if is_required:
                    if not have_default:
                        raise BadSettings(f'Required parameter "{name}" was not found')
                    else:
                        df = get_default(settings)
                        get_function_logger(f).info(f'Parameter "{name}" was not found defaulting to: "{df}"')
                        return df
            else: return f(settings)

        if registry is not None: registry[name] = _wrapped
        return _wrapped

    return _decorator



# Haskell inspired fst and snd function
fst = item(0)
snd = item(1)


# the unit or 1 operation f(x) = x
def identity(x: T.Any) -> T.Any:
    """
    The identity operation
    :param x: anything
    :return: x
    """
    return x


def circ(f: DefaultFunction, g: DefaultFunction) -> DefaultFunction:
    """
    Higher order function. Implementation of the function composition operator.
    Creates function h(x) where h(x) = f ° g = f(g(x))
    :param f: first function to compose
    :type f: Callable
    :param g: second function to compose
    :type g: Callable
    :return: the composed function
    :rtype: Callable
    """
    return lambda x: f(g(x))


def compose(*functions: DefaultFunction) -> DefaultFunction:
    """
    Reduces the variadic function arg to a composed function using the function composition operator
    :param functions: callables to compose
    :type functions: Callable
    :return: the composed function
    :rtype: Callable
    """
    return functools.reduce(circ, functions)


def apply(transform: Transform) -> functools.partial:
    """
    Applies function to an iterable. Under the hood "transform" is passed on to map as its first function argument
    using functools.partial
    :param transform: the function to apply to an iterator
    :type transform: Transform
    :return: the callable accepting the iterable
    :rtype: Callable
    """
    return functools.partial(map, transform)


def filter_by(predicate: Predicate) -> functools.partial:
    """
    Filters an iterable by the predicate. Under the hood "predicate" is passed on to filter as its first function
    argument using functools.partial
    :param predicate: the predicate to apply to filter an iterator
    :type predicate: Predicate
    :return: the callable accepting the iterable
    :rtype: Callable
    """
    return functools.partial(filter, predicate)


def sort_by(by: Transform, reverse: T.Optional[bool] = False) -> functools.partial:
    """
    Applies function to an iterable. Under the hood "by" is passed on to sorted as the "key" keyword argument
    using functools.partial
    :param by: the key function
    :type by: Transform
    :param reverse: reverse ordering (default is False)
    :type reverse: bool
    :return: the callable accepting the iterable
    :rtype: Callable
    """
    return functools.partial(sorted, key=by, reverse=reverse)


def star(function: T.Callable) -> DefaultFunction:
    """
    Unpacks a single function argument as args, to emulate the * (star) unpack operator
    :param function: to be wrapped
    :type function: Callable
    :return: a function which takes exactly one argument
    :rtype: Callable
    """
    return lambda x: function(*x)


def starstar(function: T.Callable) -> DefaultFunction:
    """
    Unpacks a single function argument as args, to emulate the ** (star) unpack operator
    :param function: to be wrapped
    :type function: Callable
    :return: a function which takes exactly one argument
    :rtype: Callable
    """
    return lambda x: function(**x)


def group_by(by: T.Optional[Transform] = identity, reverse: T.Optional[bool] = False) -> DefaultFunction:
    """
    Higher-order functions. Sorts an iterable and groups it by the "by" functions. Returns a function which
    takes an iterable
    :param by: the key function (default is identity)
    :type by: Transform
    :param reverse: reverse ordering (default is False)
    :type reverse: bool
    :return: the callable accepting the iterable
    :rtype: Callable
    """
    return compose(
        functools.partial(itertools.groupby, key=by),
        sort_by(by, reverse=reverse))


def concat(*funcs: DefaultFunction) -> DefaultFunction:
    """
    Higher-order function. Applies each of the passed function to each element and returns the results
    as a tuple. Returns a function which takes an iterable
    :param funcs: the functions to apply to an iterable
    :type funcs: Callable
    :return: the callable accepting the iterable
    :rtype: Callable
    """
    return lambda x: tuple(f(x) for f in funcs)


def foreach(action: Action, *iterables: Iterable) -> T.NoReturn:
    """
    A simple implementation of the foreach loop
    :param action: the action to apply on each of the elements in iterables
    :type action: Action
    :param iterables: a number or iterables
    :type iterables: Iterable
    """
    for iterable in iterables:
        for element in iterable:
            action(element)


def not_none(x: T.Any) -> bool:
    """
    An implementation of a predicate which checks for None values
    :param x: thing
    :return: wether x is None or not
    :rtype: bool
    """
    return x is not None


def negate(f: Predicate) -> Predicate:
    """
    Negate a predicate
    :param f: a predicate to be negated
    :type f: Callable
    :return: the negated predicate
    :rtype: Callable
    """
    return lambda x: not f(x)


def maybe(func: DefaultFunction, default=None) -> DefaultFunction:
    """
    The function to wrap which might throw an exception
    :param func: the function to wrap
    :type func: Callable
    :param default: the value passed if an exception occurs (default is None)
    :return: the wrapped function
    """

    def _inner(x):
        # check if inner is is default
        try:
            result = func(x)
        except Exception as e:
            # pass out the exception
            result = default or e
        return result

    return _inner


def eq(o: T.Any) -> Predicate:
    """
    Constructs a predicate, returning true if its argument is == o. Represents (== o)
    :param o: the object should equals to o
    :return: the predicate
    :rtype: Callable
    """
    return lambda x: x == o


def lt(o: T.Any) -> Predicate:
    """
    Constructs a predicate, returning true if its argument is < o. Represents (< o)
    :param o: the object o
    :return: the predicate
    :rtype: Callable
    """
    return lambda x: x < o


def gt(o: T.Any) -> Predicate:
    """
    Constructs a predicate, returning true if its argument is > o. Represents (> o)
    :param o: the object o
    :return: the predicate
    :rtype: Callable
    """
    return lambda x: x > o


def lte(o: T.Any) -> Predicate:
    """
    Constructs a predicate, returning true if its argument is <= o. Represents (<= o)
    :param o: the object o
    :return: the predicate
    :rtype: Callable
    """
    return lambda x: x <= o


def gte(o: T.Any) -> Predicate:
    """
    Constructs a predicate, returning true if its argument is >= o. Represents (>= o)
    :param o: the object o
    :return: the predicate
    :rtype: Callable
    """
    return lambda x: x >= o


def contains(o: T.Any) -> Predicate:
    """
    Constructs a predicate, returning true if its argument is o in x
    :param o:
    :return: the predicate
    :rtype: Callable
    """
    return lambda x: o in x


def isa(o: T.Tuple[type]) -> Predicate:
    """
    Constructs a type-checking predicate. Represents isinstance(x, o)
    :param o: the types to checked
    :type o: type
    :return: the predicate
    :rtype: Callable
    """
    return lambda x: isinstance(x, o)


def const(x: T.Any) -> DefaultFunction:
    """
    Creates a function ignoring its arguments an returns always x
    :param x: the constant values
    :return: the constant function
    :rtype: Callable
    """
    return lambda *args, **kwargs: x



class Composable:
    """
    Class to make function composition more idiomatic, by overloading __matmul__. Thus the function composition operator
    "°" -> "@". Note we do not allow for assignment. The reason is that module level defined `Composable`s might be
    modified by the user -> and thus leading to undefined behaviour. Because of multiplication precedence, left-
    associative behaviour of the function-composition operator is maintained

    "<" and ">"  are overloaded to feed the "pipes". "<" and ">" are shortcuts for the __call__ function

    The "@" and "|" operator represent left and right folding of the functional pipeline

    Example "left associative":
        ```
        c_ = Composable
        c_(f) @ g  = circ(f, g) = f ° g = f(g(x))
        c_(f) @ g @ h j = f ° g  ° h  ° j = f(g(h(j(x))))
        ```

    The right associative operator is more "intuitive" since the order resembles also the calling order

    Example "right associative" - pipe:
        ```
        c_ = Composable
        c_(f) | g  = circ(f, g) = g ° f = f(g(x))
        c_(f) | g | h | j = j ° h  ° g  ° f = f(g(h(j(x))))
        ```

    Function composition produces function. In case the composition should be evaluated, one has to put parentheses
    around it. To avoid that one can use the "<" and ">" to quickly evaluate the pipes

    Example "feeding pipes"
        ```
        (c_(identity) | apply(lambda x: x*2) | list)(range(5))
        (c_list @  apply(lambda x: x*2) @ identity)(range(5))
        # can be shortcut(ed) to
        range(5) > c_(identity) | apply(lambda x: x*2) | list
        c_list @  apply(lambda x: x*2) @ identity < range(5)
    """

    def __init__(self, *funcs: DefaultFunction):
        """
        Constructs the function composition from {funcs}
        :param funcs: the function to compose
        :type funcs: Callable
        """
        self._funcs = funcs
        self._callable = compose(*funcs)

    def __rmatmul__(self, other: DefaultFunction):
        raise NotImplementedError("Function composition does not support assignment")

    def __lmatmul__(self, other: DefaultFunction):
        raise NotImplementedError("Function composition does not support assignment")

    def __matmul__(self, other: DefaultFunction):
        return Composable(*itertools.chain(self._funcs, (other,)))

    def __call__(self, x: T.Any) -> T.Any:
        return self._callable(x)

    def __or__(self, other: DefaultFunction):
        return Composable(*itertools.chain((other,), self._funcs))

    def __lt__(self, other: T.Any) -> T.Any:
        return self.__call__(other)

    def __gt__(self, other: T.Any) -> T.Any:
        return self.__call__(other)

    @staticmethod
    def as_composable(factory: T.Callable[[T.Any], T.Callable]):
        """
        Wraps a function factory into a Composable instance
        :param factory: the function which should be made composable. Must be higher-order-function
        :type factory: Callable
        :return: the Composable object
        :rtype: Composable
        """

        # the inner function just wraps the passed one
        @functools.wraps(factory)
        def _wrapper(*args, **kwargs):
            return Composable(factory(*args, **kwargs))

        return _wrapper


# utility functions -> which might be needed more often
# non-composable versions
def apply_concat(*funcs): return apply(concat(*funcs))


apply_star = circ(apply, star)
filter_star = circ(filter_by, star)

# composable versions
c_apply = Composable.as_composable(apply)
c_filter_by = Composable.as_composable(filter_by)
c_group_by = Composable.as_composable(group_by)
c_list = Composable(list)
c_tuple = Composable(set)
c_dict = Composable(dict)
c_set = Composable(set)
c_not = Composable(lambda x: not x)
c_item = Composable.as_composable(item)
c_attr = Composable.as_composable(attr)
c_method = Composable.as_composable(method)
c_eq = Composable.as_composable(eq)
c_lt = Composable.as_composable(lt)
c_lte = Composable.as_composable(lte)
c_gt = Composable.as_composable(gt)
c_gte = Composable.as_composable(gte)
c_isa = Composable.as_composable(isa)
c_const = Composable.as_composable(const)
c_ = Composable
