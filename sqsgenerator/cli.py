
import click
from sqsgenerator.compat import available_features
from sqsgenerator.commands.run import run
from sqsgenerator.commands.export import export
from sqsgenerator.commands.params import params
from sqsgenerator.commands.analyse import analyse
from sqsgenerator.commands.compute import compute
from sqsgenerator.commands.common import pretty_format
from sqsgenerator.core import __version__, __features__


def make_version_string():
    major, minor, *_ = __version__
    return f'{major}.{minor}'


def make_repo_status():
    _, _, commit, branch = __version__
    return f'{commit}@{branch}'


_title = '[bold]sqsgenerator[/bold] - A CLI tool to find optimized SQS structures'


def make_version_message():
    return pretty_format(
        f"""{_title}
    [magenta italic]version:[/magenta italic] {make_version_string()}
    [magenta italic]build:[/magenta italic] {make_repo_status()}
    [magenta italic]features:[/magenta italic] {__features__}
    
    [magenta italic]author:[/magenta italic] Dominik Gehringer
    [magenta italic]email:[/magenta italic] dominik.gehringer@unileoben.ac.at
    [magenta italic]repo:[/magenta italic] https://github.com/dgehringer/sqsgenerator
    [magenta italic]docs:[/magenta italic] https://sqsgenerator.readthedocs.io/en/latest
    [magenta italic]modules:[/magenta italic] {available_features()}""")


@click.group(help=pretty_format(_title))
@click.version_option(prog_name='sqsgen', package_name='sqsgenerator', version=make_version_string(),
                      message=make_version_message())
def cli():
    pass


cli.add_command(run, 'run')
cli.add_command(params, 'params')
cli.add_command(export, 'export')
cli.add_command(analyse, 'analyse')
cli.add_command(compute, 'compute')


if __name__ == '__main__':
    cli()
