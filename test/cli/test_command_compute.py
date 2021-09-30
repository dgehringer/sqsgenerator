

import re
import unittest
import numpy as np
import click.testing
from sqsgenerator.cli import cli
from cli import inject_config_file
from sqsgenerator.io import read_settings_file
from sqsgenerator.public import process_settings
from sqsgenerator.settings import build_structure
from sqsgenerator.core import rank_structure, default_shell_distances


class TestComputeCommand(unittest.TestCase):

    def setUp(self):
        self.cli_runner = click.testing.CliRunner(mix_stderr=False)

    @inject_config_file()
    def test_compute_total_permutations(self):
        settings = process_settings(read_settings_file('sqs.yaml'))
        r = self.cli_runner.invoke(cli, ['compute', 'total-permutations'])
        computed_permuations = next(iter(re.findall(r'\d+', r.output)))
        self.assertEqual(settings.iterations, int(computed_permuations))
        self.assertEqual(r.exit_code, 0)

    @inject_config_file()
    def test_compute_rank(self):
        settings = process_settings(read_settings_file('sqs.yaml'))
        r = self.cli_runner.invoke(cli, ['compute', 'rank'])
        computed_rank = next(iter(re.findall(r'\d+', r.output)))
        self.assertEqual(int(computed_rank), rank_structure(settings.structure))
        self.assertEqual(r.exit_code, 0)

    @inject_config_file()
    def test_compute_distances(self):
        settings = process_settings(read_settings_file('sqs.yaml'))
        structure = build_structure(settings.composition, settings.structure[settings.which])
        r = self.cli_runner.invoke(cli, ['compute', 'shell-distances'])
        np.testing.assert_array_almost_equal(eval(r.output), default_shell_distances(structure, settings.atol, settings.rtol))
        self.assertEqual(r.exit_code, 0)

    @inject_config_file()
    def test_compute_estimated_time(self):
        r = self.cli_runner.invoke(cli, ['compute', 'estimated-time'])
        self.assertEqual(r.exit_code, 0)
        r = self.cli_runner.invoke(cli, ['compute', 'estimated-time', '--verbose'])
        self.assertEqual(r.exit_code, 0)


if __name__ == '__main__':
    unittest.main()