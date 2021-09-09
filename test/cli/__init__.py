
import click

with open('resources/cs-cl.sqs.yaml', 'r') as fh:
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