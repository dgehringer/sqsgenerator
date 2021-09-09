
import unittest
import numpy as np
from itertools import cycle
from operator import attrgetter as attr
from sqsgenerator.core import Structure, Atom
from sqsgenerator.io import read_settings_file
from sqsgenerator.settings.readers import read_structure


class TestStructure(unittest.TestCase):

    def setUp(self) -> None:
        self.raw_dict = read_settings_file('resources/cs-cl.sqs.yaml')
        self.structure = read_structure(self.raw_dict)

    def test_slicing(self):
        s = self.structure
        self.assertIsInstance(s, Structure)
        self.assertIsInstance(s[0], Atom)

        symbols_by_z = list(map(attr('symbol'), sorted(s.species, key=attr('Z'))))
        self.assertListEqual(symbols_by_z, s.sorted().symbols.tolist())

        self.assertEqual(len(symbols_by_z), len(s))

        with self.assertRaises(TypeError):
            _ = s["f"]

        with self.assertRaises(IndexError):
            _ = s[len(s)]

        sl = s[::2]
        self.assertListEqual(sl.symbols.tolist(), ['Cs']*len(sl))

        with self.assertRaises(TypeError):
            _ = s[("0", "1")]

        sl1 = s[list(range(0, len(s), 2))]
        np.testing.assert_array_equal(sl.numbers, sl1.numbers)

        alternate = cycle((True, False))

        sl2 = s[tuple(next(alternate) for _ in range(len(s)))]
        np.testing.assert_array_equal(sl.numbers, sl2.numbers)

        with self.assertRaises(TypeError):
            _ = s[np.arange(len(s)).astype(float)]

        # identitycheck

        np.testing.assert_array_equal(s.numbers, s[np.arange(len(s))].numbers)

        np.testing.assert_array_equal(s.numbers, s[np.ones(len(s)).astype(bool)].numbers)

        with self.assertRaises(ValueError):
            s.slice_with_species([])

        with self.assertRaises(ValueError):
            s.slice_with_species(['Fe'], [0, 1])

    def test_repr(self):
        length = len(self.structure)
        self.assertTrue(f'len={length}', repr(self.structure))

if __name__ == '__main__':
    unittest.main()