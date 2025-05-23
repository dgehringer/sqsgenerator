import click

from ..core import LogLevel
from ..templates import load_templates
from ._shared import render_table
from .run import run_optimization
from .version import print_version, version_string


@click.group(
    help="A simple, fast and efficient tool to find optimized Special-Quasirandom-Structures (SQS)"
)
@click.version_option(
    prog_name="sqsgen", version=version_string(), message=print_version()
)
def cli():
    pass


@cli.command(name="run")
@click.option(
    "-l",
    "--log",
    type=click.Choice(["error", "warn", "info", "debug", "trace"]),
    help="set the log value",
    default="warn",
    show_default=True,
)
@click.option(
    "-i",
    "--input",
    "_input",
    type=click.File(mode="r"),
    help="The configuration file to use",
    default="sqs.json",
    show_default=True,
)
@click.option(
    "-q",
    "--quiet",
    is_flag=True,
    default=False,
    help="Suppress progress bar and optimization info",
)
def run(_input, log: str, quiet: bool) -> None:
    match log:
        case "critical":
            log_level = LogLevel.critical
        case "warn":
            log_level = LogLevel.warn
        case "info":
            log_level = LogLevel.info
        case "debug":
            log_level = LogLevel.debug
        case "trace":
            log_level = LogLevel.trace
        case _:
            raise click.UsageError(f"Invalid log level: {log!r}")

    run_optimization(_input.read(), log_level=log_level, quiet=quiet)


@cli.command(
    name="template",
    help="use templates to create a new configuration file quickly",
)
@click.argument(
    "name",
    required=False,
    type=click.Choice(list(load_templates().keys())),
    metavar="NAME",
)
def template(name: str | None) -> None:
    if name is None:

        def format_author(authors: list[dict[str, str]]) -> str:
            if authors:
                first_author = authors[0]
                if "name" in first_author and "surname" in first_author:
                    return "{} {} {}".format(
                        first_author["name"],
                        first_author["surname"],
                        (
                            "({})".format(first_author["email"])
                            if "email" in first_author
                            and first_author["email"] is not None
                            else ""
                        ),
                    )
            return ""

        render_table(
            [
                (
                    template_name,
                    format_author(template.get("authors", [])),
                    ", ".join(template.get("tags", [])),
                )
                for template_name, template in load_templates().items()
            ],
            "NAME",
            "AUTHOR(S)",
            "TAGS",
            sep=" ",
            NAME=dict(fg="cyan", bold=True),
        )


if __name__ == "__main__":
    cli()
