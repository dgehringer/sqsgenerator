import pprint
import random
import unittest
import numpy.testing
from functools import reduce
from collections import Counter
from math import factorial as f, floor
from operator import mul, attrgetter as attr
from sqsgenerator import read_settings_file, sqs_optimize, make_rank, process_settings, rank_structure, \
    total_permutations, sqs_analyse
from sqsgenerator.settings.readers import read_structure


class TestUtils(unittest.TestCase):

    def setUp(self) -> None:
        self.raw_dict = read_settings_file('resources/cs-cl.sqs.yaml')
        self.settings = process_settings(self.raw_dict)
        self.structure = read_structure(self.raw_dict)
        self.results, _ = sqs_optimize(self.raw_dict, make_structures=True, similar=True, minimal=False)

    def test_rank_and_unrank(self):
        for rank, data in self.results.items():
            created_structure = make_rank(self.settings.structure, rank, configuration=data['configuration'], which=self.settings.which)
            self.assertEqual(data['structure'], created_structure)
            self.assertEqual(rank_structure(created_structure, which=self.settings.which), rank)

    def test_rank_structure(self):
        sorted_structure = self.structure.sorted()
        self.assertEqual(rank_structure(sorted_structure), 1)

        reversed_structure = sorted_structure.with_species(reversed(sorted_structure.symbols))
        self.assertEqual(rank_structure(reversed_structure), total_permutations(sorted_structure))
        self.assertEqual(total_permutations(sorted_structure), total_permutations(reversed_structure))

    def test_total_permutations(self):
        hist = Counter(self.structure.symbols)
        self.assertEqual(total_permutations(self.structure), f(sum(hist.values()))/reduce(mul, map(f, hist.values())))

    def test_analyse(self):
        from sqsgenerator.core import available_species, make_supercell, build_configuration
        symbols = list(map(attr('symbol'), available_species()))
        num_species = random.randint(2, 4)
        scale = 2

        num_atoms = len(self.structure)
        superstructure = make_supercell(self.structure, scale, scale, scale)

        total_num_of_atoms = scale*scale*scale*num_atoms

        composition = {random.choice(symbols): random.randint(1, floor(total_num_of_atoms/num_species)) for _ in range(num_species-1)}
        composition[random.choice(list(set(symbols) - set(composition.keys())))] = total_num_of_atoms - sum(composition.values())

        assert sum(composition.values()) == total_num_of_atoms
        settings = dict(
            structure = superstructure,
            composition = composition,
            iterations=1e4
        )

        results, timinigs = sqs_optimize(settings, similar=True, minimal=False, make_structures=True)
        pprint.pprint(timinigs)

        for rank, data in results.items():
            analysed = sqs_analyse([data['structure']], settings=settings)
            self.assertTrue(rank in analysed)
            analysed = analysed[rank]
            self.assertEqual(rank_structure(data['structure']), rank)
            numpy.testing.assert_array_almost_equal(analysed['parameters'], data['parameters'])
            self.assertAlmostEqual(data['objective'], analysed['objective'])
