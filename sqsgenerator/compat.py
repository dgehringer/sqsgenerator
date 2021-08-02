import logging
import operator
import functools
import frozendict
import typing as T

from enum import Enum
import logging


class Feature(Enum):
    ase = 'ase'
    pymatgen = 'pymatgen'
    rich = 'rich'
    pyiron = 'pyiron'


class FeatureNotAvailableException(Exception):
    pass


__features = None


def is_initialized():
    return __features is not None


def _is_feature_available(feature: Feature):
    try:
        __import__(feature.value)
    except ImportError: return False
    else: return True


def _check_features():
    global __features
    if  __features is None:
        self_module = __import__(__name__)
        __features = frozendict.frozendict(map(lambda f: (f.value, _is_feature_available(f)), Feature))
        for feat in Feature:
            getter = functools.partial(operator.itemgetter(feat.value), __features)
            setattr(self_module, f'have_{feat.value}', getter)
            message = f'Feature "{feat.value}" was ' + ('found' if __features[feat.value] else 'not found')
            logging.getLogger(f'compat.check_features').info(message)


def have_feature(feature: T.Union[Feature, str]) -> bool:
    assert is_initialized()
    freature_value = feature.value if isinstance(feature, Feature) else feature
    return freature_value in __features and __features[freature_value]


def require(*features: Feature, condition=all):

    def _decorator(f: T.Callable):
        assert is_initialized()

        @functools.wraps(f)
        def _wrapped(*args, **kwargs):
            if not condition(map(have_feature, features)): raise FeatureNotAvailableException(features)
            return f(*args, **kwargs)
        return _wrapped

    return _decorator

_check_features()