import pprint
import unittest
from sqsgenerator import read_settings_file, sqs_optimize, make_rank, process_settings, rank_structure
from sqsgenerator.settings.readers import read_structure


class TestUtils(unittest.TestCase):

    def setUp(self) -> None:
        self.raw_dict = read_settings_file('resources/cs-cl.sqs.yaml')
        self.settings = process_settings(self.raw_dict)
        self.structure = read_structure(self.raw_dict)
        self.results, _ = sqs_optimize(self.raw_dict, make_structures=True, similar=True, minimal=False)

    def test_rank_and_unrank(self):
        for rank, data in self.results.items():
            created_structure = make_rank(self.settings.structure, data['configuration'], rank, which=self.settings.which)
            self.assertEqual(data['structure'], created_structure)
            self.assertEqual(rank_structure(created_structure, which=self.settings.which), rank)