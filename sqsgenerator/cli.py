
import click
from sqsgenerator.commands.params import params


def make_version_string():
    from sqsgenerator.core import __version__
    major, minor, *_ = __version__
    return f'{major}.{minor}'


def make_version_message():
    from sqsgenerator.core import __version__, __features__
    _, _, hash_, branch = __version__
    message = f'%(prog)s, version %(version)s, commit {hash_}@{branch}, features {":".join(__features__)}'
    return message


@click.group(help='sqsgenerator')
@click.version_option(prog_name='sqsgenerator', version=make_version_string(), message=make_version_message())
def cli():
    pass

cli.add_command(params, 'params')

if __name__ == '__main__':
    cli()