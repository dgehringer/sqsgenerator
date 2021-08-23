import os
import random
import unittest
import attrdict
import numpy as np
from sqsgenerator.adapters import to_ase_atoms, to_pymatgen_structure
from sqsgenerator.io import read_settings_file
from sqsgenerator.settings.readers import read_atol, \
    read_rtol, \
    read_mode, \
    read_structure, \
    read_iterations, \
    read_composition, \
    read_max_output_configurations, \
    BadSettings, ATOL, RTOL, IterationMode, Structure


def settings(recursive=True, **kwargs):
    return attrdict.AttrDict({**kwargs}, recursive=recursive)


def test_function(test_f):

    def test_f_wrapper(**kwargs):
        return test_f(settings(**kwargs))

    def _decorator(f):

        def _inner(self):
            return f(self, test_f_wrapper)

        return _inner

    return _decorator


class TestSettingReaders(unittest.TestCase):

    def setUp(self) -> None:
        self.raw_dict = read_settings_file('examples/cs-cl.sqs.yaml')
        self.raw_dict_from_file = read_settings_file('examples/cs-cl.poscar.sqs.yaml')
        self.file_name = self.raw_dict_from_file.structure.file
        self.structure = read_structure(self.raw_dict)

    def assertStructureEquals(self, s1: Structure, s2: Structure, prec=3):
        self.assertEqual(s1.num_unique_species, s2.num_unique_species)
        self.assertTrue(np.allclose(s1.numbers, s2.numbers))
        coords_close = np.allclose(np.round(s1.frac_coords, prec), np.round(s2.frac_coords, prec))
        if not coords_close:
            print(np.abs(s1.frac_coords - s2.frac_coords))
        self.assertTrue(coords_close)

    @test_function(read_atol)
    def test_read_atol(self, f):
        self.assertAlmostEqual(f(), ATOL)
        self.assertAlmostEqual(f(atol=1.5), 1.5)
        with self.assertRaises(BadSettings):
            f(atol=-1)
        with self.assertRaises(BadSettings):
            f(atol="adsfasdf")

    @test_function(read_rtol)
    def test_read_rtol(self, f):
        self.assertAlmostEqual(f(), RTOL)
        self.assertAlmostEqual(f(rtol=1.5), 1.5)
        with self.assertRaises(BadSettings):
            f(rtol=-1)

    @test_function(read_mode)
    def test_read_mode(self, f):
        for mode, obj in IterationMode.names.items():
            self.assertEqual(f(mode=mode), obj)
            self.assertEqual(f(mode=obj), obj)

        with self.assertRaises(BadSettings):
            f(mode='atol')

        self.assertEqual(f(), IterationMode.random)

    @test_function(read_iterations)
    def test_read_iterations(self, f):

        self.assertEqual(f(mode=IterationMode.random), 1e5)
        self.assertEqual(f(mode=IterationMode.systematic), -1)

        num_iterations = random.randint(1000, 10000)
        self.assertEqual(f(mode=IterationMode.systematic, iterations=num_iterations), num_iterations)
        self.assertEqual(f(mode=IterationMode.random, iterations=num_iterations), num_iterations)

        with self.assertRaises(BadSettings):
            # raise a TypeError in convert
            f(iterations=())

        with self.assertRaises(BadSettings):
            # raise a ValueError in convert
            f(iterations="adsfasdf")

        with self.assertRaises(BadSettings):
            # raise a TypeError in convert
            f(iterations=-23)

    @test_function(read_max_output_configurations)
    def test_read_max_output_configurations(self, f):

        self.assertEqual(f(), 10)
        self.assertEqual(f(max_output_configurations=1000), 1000)
        self.assertEqual(f(max_output_configurations=1e3), 1000)

        with self.assertRaises(BadSettings):
            # raise a TypeError in convert
            f(max_output_configurations=())

        with self.assertRaises(BadSettings):
            # raise a ValueError in convert
            f(max_output_configurations="adsfasdf")

        with self.assertRaises(BadSettings):
            # raise a TypeError in convert
            f(max_output_configurations=-23)

    @test_function(read_structure)
    def test_read_structure(self, f):
        self.assertStructureEquals(f(structure=self.structure), self.structure)
        self.assertStructureEquals(f(structure=self.structure), self.structure)
        self.assertStructureEquals(f(**self.raw_dict_from_file), self.structure)
        self.assertStructureEquals(f(structure=to_ase_atoms(self.structure)), self.structure)
        self.assertStructureEquals(f(structure=to_pymatgen_structure(self.structure)), self.structure)
        with self.assertRaises(BadSettings):
            f()
        with self.assertRaises(BadSettings):
            f(structure={'A': 1})


    @test_function(read_composition)
    def test_read_composition(self, f):

        with self.assertRaises(BadSettings):
            # raise a TypeError in convert
            f(structure=self.structure, composition={})

        with self.assertRaises(BadSettings):
            # raise a wrong number of total atoms
            f(structure=self.structure, composition=dict(Fr=18, Lu=18))

        with self.assertRaises(BadSettings):
            # correct number but less than one
            f(structure=self.structure, composition=dict(Fr=54, Lu=0))

        with self.assertRaises(BadSettings):
            # correct number but negative number
            f(structure=self.structure, composition=dict(Fr=55, Lu=-1))

        with self.assertRaises(BadSettings):
            # wrong species
            f(structure=self.structure, composition=dict(Fr=27, Kf=27))

        with self.assertRaises(BadSettings):
            # type error in atom number
            f(structure=self.structure, composition=dict(Fr=27, Na='asdf'))

        with self.assertRaises(BadSettings):
            # wrong number of atoms on sublattice
            f(structure=self.structure, composition=dict(Fr=27, Kf=27, which='Cs'))

        with self.assertRaises(BadSettings):
            # non existing sublattice
            f(structure=self.structure, composition=dict(Fr=14, K=13, which='Na'))

        with self.assertRaises(BadSettings):
            # wrong type in sublattice specification
            f(structure=self.structure, composition=dict(Fr=27, Na=27, which=345345))

        with self.assertRaises(BadSettings):
            # index out of bounds in sublattice specification
            f(structure=self.structure, composition=dict(Fr=2, Na=2, which=(0,1,2,4, self.structure.num_atoms+4)))

        with self.assertRaises(BadSettings):
            # index out of bounds in sublattice specification
            f(structure=self.structure, composition=dict(Fr=2, Na=2, which=(0, 1, 2, 4, 'g')))

        with self.assertRaises(BadSettings):
            # too few atoms on sublattice
            f(structure=self.structure, composition=dict(Fr=0, Na=1, which=(0,)))

        with self.assertRaises(BadSettings):
            # wrong number of atoms on sublattice
            f(structure=self.structure, composition=dict(Fr=27, Kf=27, which='all'))

        s = settings(structure=self.structure, composition=dict(Cs=27, Cl=27, which='all'))
        read_composition(s)
        self.assertTrue('is_sublattice' in s)
        self.assertFalse(s.is_sublattice)

        sublattice = 'Cs'
        s = settings(structure=self.structure, composition=dict(H=13, He=14, which=sublattice))
        read_composition(s)
        self.assertTrue('is_sublattice' in s)
        self.assertTrue(s.is_sublattice)

if __name__ == '__main__':
    unittest.main()