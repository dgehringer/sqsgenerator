
import os
import unittest
import click.testing
from cli import inject_config_file
from sqsgenerator.io import compression_to_file_extension
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

    @inject_config_file()
    def test_run_iteration_export_compression(self):
        for compression in compression_to_file_extension.keys():

            r = self.cli_runner.invoke(cli, ['run', 'iteration', '--export', '--no-minimal', '--similar', '--compress', compression])
            archive_name = f'sqs.{compression_to_file_extension.get(compression)}'

            if r.exit_code != 0:
                print(r.output, r.exit_code, r.stderr)

            self.assertEqual(r.exit_code, 0)
            self.assertTrue(os.path.exists(archive_name))

    @inject_config_file()
    def test_params_check(self):
        pass


if __name__ == '__main__':
    unittest.main()