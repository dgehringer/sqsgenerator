import yaml
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

    @inject_config_file()
    def test_params_show(self):
        r = self.cli_runner.invoke(cli, ['params', 'show'])
        self.assertEqual(r.exit_code, 0)

        r = self.cli_runner.invoke(cli, ['params', 'show', '-of', 'yaml'])
        self.assertEqual(r.exit_code, 0)
        reloaded = yaml.safe_load(r.stdout)
        need_fields = {'atol', 'rtol', 'mode', 'pair_weights', 'shell_distances', 'shell_weights', 'threads_per_rank', 'max_output_configurations'}
        self.assertTrue(all(f in reloaded for f in need_fields))

    @inject_config_file()
    def test_params_check(self):
        r = self.cli_runner.invoke(cli, ['params', 'check'])
        self.assertEqual(r.exit_code, 0)


if __name__ == '__main__':
    unittest.main()