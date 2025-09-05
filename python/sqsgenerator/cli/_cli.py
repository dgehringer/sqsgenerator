import functools
import io
import json
import os
from types import MappingProxyType
from typing import Optional

import click

from .._adapters import available_formats, read, write
from ..core import Atom, LogLevel, Prec, load_result_pack
from ..templates import load_templates
from ._link import link as _link
from ._run import run_optimization
from ._shared import format_hyperlink, render_error, render_table
from ._version import print_version, version_string

_LOG_LEVELS: MappingProxyType[str, LogLevel] = MappingProxyType(
    {
        "trace": LogLevel.trace,
        "debug": LogLevel.debug,
        "info": LogLevel.info,
        "warn": LogLevel.warn,
        "error": LogLevel.error,
    }
)


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
    type=click.Choice(list(_LOG_LEVELS.keys())),
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
    log_level = _LOG_LEVELS.get(log)
    if log_level is None:
        raise click.UsageError(f"Invalid log level: {log!r}")

    if (
        result := run_optimization(_input.read(), log_level=log_level, quiet=quiet)
    ) is not None:
        path, ext = os.path.splitext(_input.name)
        with open(f"{path}.mpack", "wb") as output_file:
            output_file.write(result.bytes())


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
def template(name: Optional[str]) -> None:
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
    else:
        buf = io.StringIO()
        print_ = functools.partial(print, file=buf)
        print_(
            click.style("Template: ", bold=True)
            + click.style(name, fg="cyan", bold=True)
        )
        print_()
        print_(
            click.style("Description: ", bold=True)
            + (template := load_templates()[name])["description"]
        )

        def render_author(author: dict[str, str]) -> None:
            name, surname = author["name"], author["surname"]
            print_("  " + click.style("Name: ", italic=True) + f"{name} {surname}")
            if (email := author.get("email")) is not None:
                print_(
                    "  "
                    + click.style("Mail: ", italic=True)
                    + format_hyperlink(email, f"mailto:{email}")
                )
            print_()

        if authors := template.get("authors", []):
            print_(click.style("Authors:", bold=True))
            for author in authors:
                render_author(author)
        click.echo(buf.getvalue())
        with open(f"{name}.sqs.json", "w") as output_file:
            json.dump(template["config"], output_file, indent=2)


@cli.group()
@click.option(
    "--output",
    "-o",
    type=click.File(mode="rb"),
    default="sqs.mpack",
    help="The output file from which structures should be exported from",
)
@click.pass_context
def output(ctx: click.Context, output: str) -> None:
    ctx.obj = output


@output.command(name="list")
@click.pass_obj
def _list(output: click.File) -> None:
    pack = load_result_pack(output.read(), prec=Prec.double)
    buf = io.StringIO()
    print_ = functools.partial(print, file=buf)
    print_(
        click.style("Mode: ", bold=True)
        + click.style(pack.config.iteration_mode.name, italic=True)
    )
    print_(click.style("min(O(σ)): ", bold=True) + f"{pack.best().objective:.5f}")
    print_(click.style("Num. objectives:  ", bold=True) + f"{pack.num_objectives()}")
    print_()
    render_table(
        [
            (f"{index}", f"{objective:.5f}", f"{len(results)}")
            for index, (objective, results) in enumerate(pack)
        ],
        "INDEX",
        "OBJ.",
        "N",
        sep="  ",
        INDEX=dict(fg="cyan", bold=True),
        buf=buf,
    )


@output.command(name="structure")
@click.pass_obj
@click.option(
    "--objective",
    type=click.IntRange(min=0),
    default=[0],
    multiple=True,
    show_default=True,
    help="select the n-th best objective. This is value is specified as an index. This argument can be specified multiple times to export multiple structures.",
)
@click.option(
    "--index",
    "-i",
    type=click.IntRange(min=0),
    default=[0],
    multiple=True,
    show_default=True,
    help="the index of the structure to export,  specified by the --objective option. This argument can be specified multiple times to export multiple structures.",
)
@click.option(
    "--format",
    "-f",
    "fmt",
    type=click.Choice(available_formats()),
    default="cif",
    show_default=True,
    help="the output format of the structure",
)
def structure(
    output: click.File, objective: tuple[int, ...], index: tuple[int, ...], fmt: str
) -> None:
    pack = load_result_pack(output.read(), prec=Prec.double)
    for obj in objective:
        if not (0 <= obj < pack.num_objectives()):
            render_error(
                f"Invalid objective index '{obj}'",
                info=f"objective index must be between 0 and {pack.num_objectives() - 1}",
            )
        else:
            _, results = pack[obj]
            for idx in index:
                if not (0 <= idx < len(results)):
                    render_error(
                        f"Invalid structure index '{idx}' for objective {obj}",
                        info=f"structure index must be between 0 and {len(results) - 1} for objective {obj}",
                    )
                else:
                    write(results[idx].structure(), f"sqs-{obj}-{idx}.{fmt}")


@output.command(name="config")
@click.pass_obj
def config(output: click.File) -> None:
    pack = load_result_pack(output.read(), prec=Prec.double)

    parsed = json.loads(pack.config.json())
    # fixup composition
    for composition in parsed["composition"]:
        composition["composition"] = {
            Atom.from_z(z).symbol: count for z, count in composition["composition"]
        }
    parsed["shell_weights"] = [
        {f"{num}": weight for num, weight in shell_weights}
        for shell_weights in parsed["shell_weights"]
    ]

    stem, ext = os.path.splitext(output.name)

    with open(f"{stem}.config.json", "w") as config_file:
        json.dump(parsed, config_file, indent=2)


@cli.command(
    name="link",
    help="create a shareable link for the configuration file on https://sqsgen.gehringer.tech",
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
    "--open",
    "open_in_browser",
    is_flag=True,
    default=False,
    help="Open the link in a web browser",
)
def link(_input: click.File, open_in_browser: bool) -> None:
    _link(_input.read(), open_in_browser)
