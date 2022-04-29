
import os
import pprint

import yaml
import glob
import unittest
import numpy as np
import click.testing
from cli import inject_config_file
from sqsgenerator.public import process_settings, AttrDict
from sqsgenerator.io import compression_to_file_extension, read_settings_file
from sqsgenerator.cli import cli


class TestRunIterationCommand(unittest.TestCase):

    def setUp(self):
        self.cli_runner = click.testing.CliRunner(mix_stderr=False)

    @inject_config_file()
    def test_run_iteration_simple(self):
        r = self.cli_runner.invoke(cli, ['run', 'iteration'])
        self.assertTrue(os.path.exists('sqs.result.yaml'))
        self.assertEqual(r.exit_code, 0)

    @inject_config_file()
    def test_run_iteration_export(self):
        result_file = 'sqs.result.yaml'
        r = self.cli_runner.invoke(cli, ['run', 'iteration', '--export', '--no-minimal', '--similar'])
        self.assertTrue(os.path.exists(result_file))
        self.assertEqual(r.exit_code, 0)

        os.remove(result_file)

        # try to make a export with a unreasonable export format
        r = self.cli_runner.invoke(cli, ['run', 'iteration', '--export', '--no-minimal', '--similar', '--format', 'a123'])
        self.assertTrue(os.path.exists(result_file))
        self.assertNotEqual(r.exit_code, 0)

        r = self.cli_runner.invoke(cli, ['export'])
        self.assertTrue(os.path.exists(result_file))
        self.assertEqual(r.exit_code, 0)

        r = self.cli_runner.invoke(cli, ['export', 'sqs.yaml'])
        self.assertNotEqual(r.exit_code, 0)

    @inject_config_file()
    def test_run_iteration_export_compression(self):
        for compression in compression_to_file_extension.keys():

            r = self.cli_runner.invoke(cli, ['run', 'iteration', '--export', '--no-minimal', '--similar', '--compress', compression])
            archive_name = 'sqs.{}'.format(compression_to_file_extension.get(compression))

            if r.exit_code != 0:
                print(r.output, r.exit_code, r.stderr)

            self.assertEqual(r.exit_code, 0)
            self.assertTrue(os.path.exists(archive_name))

    @inject_config_file()
    def no_test_analyse_command(self):
        result_file = 'sqs.result.yaml'
        r = self.cli_runner.invoke(cli, ['run', 'iteration', '--export', '--no-minimal', '--similar', '--dump-include',
                                         'parameters', '--dump-include', 'objective'])
        self.assertTrue(os.path.exists(result_file))
        self.assertEqual(r.exit_code, 0)

        results_from_iteration = read_settings_file(result_file).configurations

        r = self.cli_runner.invoke(cli, ['export'])

        optimized_structures = glob.glob('*.cif')
        print('FOUND EXPORTED_STRUCTURES', optimized_structures)

        conf = dict(read_settings_file('sqs.yaml'))
        proc_conf = process_settings(AttrDict(conf))
        conf['which'] = proc_conf['which']


        with open('sqs.mod.yaml', 'w') as h:
            yaml.safe_dump(conf, h)

        r = self.cli_runner.invoke(cli, ['analyse'] + optimized_structures + ['-of', 'yaml', '--settings', 'sqs.mod.yaml'])
        print('STDERR', r.exc_info)
        self.assertEqual(r.exit_code, 0)
        results_from_analyse = yaml.safe_load(r.output)
        ranks = sorted([int(fname.split('.cif')[0]) for fname in results_from_analyse])

        self.assertListEqual(ranks, list(results_from_iteration.keys()))

        for k in ranks:
            iteration, analyse = results_from_iteration[k], next(iter(results_from_analyse[f'{k}.cif'].values()))

            self.assertAlmostEqual(iteration['objective'], analyse['objective'])
            np.testing.assert_array_almost_equal(iteration['parameters'], analyse['parameters'])


if __name__ == '__main__':
    unittest.main()