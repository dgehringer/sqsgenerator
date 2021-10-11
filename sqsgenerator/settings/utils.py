
import numbers
import numpy as np
import typing as T
from functools import partial
from sqsgenerator.settings.exceptions import BadSettings
from sqsgenerator.core import symbols_from_z, z_from_symbols, Structure, ALL_SITES, build_configuration as build_configuration_


def ensure_array_shape(o: np.ndarray, shape: tuple, msg: T.Optional[str] = None):
    if o.shape != shape:
        raise (BadSettings(msg) if msg is not None else BadSettings)


def ensure_array_symmetric(o: np.ndarray, msg: T.Optional[str] = None):
    error = BadSettings(msg) if msg is not None else BadSettings
    if o.ndim == 2:
        if not np.allclose(o, o.T):
            raise error
    elif o.ndim == 3:
        for i, oo in enumerate(o, start=1):
            if not np.allclose(oo, oo.T):
                raise error
    return True


def int_safe(x):
    return int(float(x))


def convert(o, to=int, converter=None, on_fail=BadSettings, message=None):
    if isinstance(o, to):
        return o
    try:
        r = (to if converter is None else converter)(o)
    except (ValueError, TypeError):
        if on_fail is not None:
            raise on_fail(message.format(o)) if message is not None else on_fail()
    else:
        return r


def symbol_to_z(symbol: str):
    try:
        return next(iter(z_from_symbols([symbol])))
    except ValueError as e:
        # is raised by the extension, however we forward it as BadSettings Error
        raise BadSettings(e)


def to_internal_composition_specs(composition: dict, structure: Structure):
    initial_species = structure.unique_species
    parse_number_of_atoms = partial(convert, to=int, message='Failed to interpret "{0}" as a number of atoms')
    # there is the chance that a parser returns "0" species as integer, we have to consider this responsibilities

    def symbol_to_z_with_zero(k: T.Union[str, int]):
        if isinstance(k, int):
            if k == 0:
                return k
            else:
                raise BadSettings('I can only iterpret "0" as an atomic species')
        else:
            return symbol_to_z(k)

    def parse_value(v: T.Union[int, dict]):
        if isinstance(v, numbers.Number):
            v = parse_number_of_atoms(v)
            if v < 1:
                raise BadSettings('You have distribute at least one atom')
            return {ALL_SITES: v}
        elif isinstance(v, dict):
            if not all(s in initial_species for s in v.keys()):
                raise BadSettings('You cannot distribute atoms on a sublattice which is not specified in the initial '
                                  'structure')
            return {symbol_to_z_with_zero(s): parse_number_of_atoms(n) for s, n in v.items()}
        else:
            raise BadSettings('Cannot interpret composition specification')

    return {symbol_to_z_with_zero(s): parse_value(v) for s, v in composition.items()}


def build_structure(composition: dict, structure: Structure):
    composition = to_internal_composition_specs(composition, structure)
    # after parsing we check if the internal routine runs without crashing, this is necessare since
    # "build_configuration" get's called in the IterationSettings constructor
    try:
        *_, conf = build_configuration_(structure.numbers.tolist(), composition)
    except (RuntimeError, ValueError) as e:
        # forward the error as BadSettings
        raise BadSettings(e)

    return structure.with_species(symbols_from_z(conf))