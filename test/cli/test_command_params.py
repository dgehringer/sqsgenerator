import json
import yaml
import pickle
import unittest
import click.testing
from sqsgenerator.cli import cli


with open('examples/cs-cl.sqs.yaml', 'r') as fh:
    config_contents = fh.read()


def inject_config_file(filename='sqs.yaml'):

    def _decorator(f):

        def _injected(self, *args, **kwargs):
            assert hasattr(self, 'cli_runner')
            assert isinstance(self.cli_runner, click.testing.CliRunner)
            with self.cli_runner.isolated_filesystem():
                with open(filename, 'w') as fh:
                    fh.write(config_contents)
                return f(self, *args, **kwargs)

        return _injected

    return _decorator


class TestParamsCommand(unittest.TestCase):

    def setUp(self):
        self.cli_runner = click.testing.CliRunner(mix_stderr=False)


    def assert_have_needed_fields(self, reloaded):
        need_fields = {'atol', 'rtol', 'mode', 'pair_weights', 'shell_distances', 'shell_weights', 'threads_per_rank',
                       'max_output_configurations'}

        for field in need_fields:
            self.assertIn(field, reloaded)

    def input_output_loop(self, format, loader):
        output_prefix = 'sqs.test'
        r = self.cli_runner.invoke(cli, ['params', 'show', '-of', format])
        self.assertEqual(r.exit_code, 0)
        if format == 'pickle':
            print(f"\"{r.stdout}\"")
        reloaded = loader(r.stdout)
        self.assert_have_needed_fields(reloaded)

        filename = f'{output_prefix}.{format}'
        with open(filename, 'w') as fh:
            fh.write(r.stdout)

        r = self.cli_runner.invoke(cli, ['params', 'show', filename , '-if', format])
        self.assertEqual(r.exit_code, 0)

    @inject_config_file()
    def test_params_show(self):

        r = self.cli_runner.invoke(cli, ['params', 'show'])
        self.assertEqual(r.exit_code, 0)

        self.input_output_loop('yaml', yaml.safe_load)
        self.input_output_loop('json', json.loads)

    @inject_config_file()
    def test_params_check(self):
        r = self.cli_runner.invoke(cli, ['params', 'check'])
        self.assertEqual(r.exit_code, 0)

        r = self.cli_runner.invoke(cli, ['params', 'check', 'sqs1.yaml'])
        self.assertNotEqual(r.exit_code, 1)


if __name__ == '__main__':
    unittest.main()