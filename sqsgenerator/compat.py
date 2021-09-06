"""
Provides utility functions and accsess to optional modules
"""

import sys
import logging
import operator
import functools
import frozendict
import typing as T
from enum import Enum


class Feature(Enum):
    ase = 'ase'
    pymatgen = 'pymatgen'
    rich = 'rich'
    pyiron = 'pyiron'
    mpi4py = 'mpi4py'
    json = 'json'
    yaml = 'yaml'
    pickle = 'pickle'


class FeatureNotAvailableException(Exception):
    pass


__features = None


def get_module(feature: Feature):
    return sys.modules[feature.value]


def is_initialized():
    return __features is not None


def _is_feature_available(feature: Feature):
    try:
        __import__(feature.value)
    except ImportError:
        return False
    else:
        return True


def _check_features():
    global __features
    if __features is None:
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
        def _wrapper(*args, **kwargs):
            if not condition(map(have_feature, features)): raise FeatureNotAvailableException(features)
            return f(*args, **kwargs)
        return _wrapper

    return _decorator


def have_mpi_support():
    import sqsgenerator.core.core
    return 'mpi' in sqsgenerator.core.core.__features__


def available_features():
    return tuple(feature.value for feature in Feature if have_feature(feature))


def available_features_with_version():
    default_version = lambda _ : ''
    module_version_attr = lambda f: f'-{get_module(f).__version__}'
    verstion_getters = {
        Feature.ase: module_version_attr,
        Feature.pymatgen: module_version_attr,
        Feature.pyiron: module_version_attr,
        Feature.yaml: module_version_attr,
        Feature.mpi4py: module_version_attr
    }
    return tuple(f'{feature.value}{verstion_getters.get(feature, default_version)(feature)}' for feature in Feature)


_check_features()