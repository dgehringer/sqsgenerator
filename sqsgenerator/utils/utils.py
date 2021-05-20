import sys
import unicodedata
import logging
from .termcolor import colored

try:
    from math import isclose
except:
    isclose = lambda a, b, rel_tol=1e-09, abs_tol=0.0: abs(a - b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)

ERROR = logging.ERROR
WARNING = logging.WARNING
DEBUG = logging.DEBUG
INFO = logging.INFO
unicode_alpha = unicodedata.lookup('GREEK SMALL LETTER ALPHA')
unicode_capital_sigma = unicodedata.lookup('GREEK CAPITAL LETTER SIGMA')

subscript_one = unicodedata.lookup('SUBSCRIPT ONE')
subscript_two = unicodedata.lookup('SUBSCRIPT TWO')
subscript_three = unicodedata.lookup('SUBSCRIPT THREE')
subscript_four = unicodedata.lookup('SUBSCRIPT FOUR')
subscript_five = unicodedata.lookup('SUBSCRIPT FIVE')
subscript_six = unicodedata.lookup('SUBSCRIPT SIX')
subscript_seven = unicodedata.lookup('SUBSCRIPT SEVEN')
subscript_eight = unicodedata.lookup('SUBSCRIPT EIGHT')
subscript_nine = unicodedata.lookup('SUBSCRIPT NINE')
subscript_zero = unicodedata.lookup('SUBSCRIPT ZERO')

subscript_mapping = {
    0: subscript_zero,
    1: subscript_one,
    2: subscript_two,
    3: subscript_three,
    4: subscript_four,
    5: subscript_five,
    6: subscript_six,
    7: subscript_seven,
    8: subscript_eight,
    9: subscript_nine
}


def star(f):
  return lambda args: f(*args)


def parse_float(string, except_val=None, raise_exc=False):
    if raise_exc:
        return float(string)
    else:
        try:
            return float(string)
        except (ValueError, TypeError):
            return except_val


def parse_separated_string(string, delimiter=','):
    return list(
        filter(
            lambda string_: not string_.strip().isspace() and string_ is not '',
            string.split(delimiter)
        )
    )


def write_message(message, level=logging.ERROR, exit=False):
    color_mapping = {
        logging.ERROR: 'red',
        logging.WARNING: 'yellow',
        logging.DEBUG: 'blue',
        logging.INFO: 'green'
    }
    text = colored(message, color=color_mapping[level])
    logging.log(level, text)
    if exit:
        sys.exit()


def get_superscript(shell):
    result = ""
    for digit in str(shell):
        result += subscript_mapping[int(digit)]
    return result


def full_name(thing):
    return thing.__module__+'.'+thing.__class__.__name__


def all_subclasses(cls):
    return cls.__subclasses__() + [g for s in cls.__subclasses__() for g in all_subclasses(s)]


def ilen(it):
    return sum(1 for _ in it)