import io
import threading
from typing import Any

import click

from .core import LogLevel, __build__, __version__, optimize, parse_config


def _format_hyperlink(text, link: str) -> str:
    return f"\033]8;;{link}\007{text}\033]8;;\007"


def _version_string() -> str:
    """Format the version string."""
    major, minor, build = __version__
    return f"{major}.{minor}.{build}"


def print_version() -> str:
    """Print the version of the package."""
    buf = io.StringIO()

    def write(text: str, **kwargs: Any) -> None:
        buf.write(click.style(text, **kwargs))

    write("sqsgen", bold=True, italic=True)
    write(
        " - A simple, fast and efficient tool to find optimized Special-Quasirandom-Structures (SQS)\n"
    )
    write("\n")

    def print_row(name: str, value: str, hyperlink: str | None = None) -> None:
        write(f"{name}:", bold=True, fg="green")
        write(
            f" {value if hyperlink is None else _format_hyperlink(value, hyperlink)}\n"
        )

    print_row("Version", _version_string())
    branch, commit = __build__
    print_row(
        "Build Info",
        f"{branch}@{commit}",
        f"https://github.com/dgehringer/sqsgenerator/commit/{commit}",
    )

    print_row(
        "Publication (DOI)",
        "10.1016/j.cpc.2023.108664",
        "https://doi.org/10.1016/j.cpc.2023.108664",
    )
    print_row(
        "Repository",
        "dgehringer/sqsgenerator",
        "https://github.com/dgehringer/sqsgenerator",
    )
    print_row("Author", "Dominik Gehringer")
    print_row(
        "Docs",
        "https://sqsgenerator.readthedocs.io/en/latest",
        "https://sqsgenerator.readthedocs.io/en/latest",
    )

    features: list[str] = []
    try:
        from pymatgen.core import __version__

        features.append(f"pymatgen@{__version__}")
    except ImportError:
        pass
    try:
        from ase import __version__

        features.append(f"ase@{__version__}")
    except ImportError:
        pass

    if features:
        print_row("Additional", ", ".join(features))

    return buf.getvalue()


@click.command(
    name="sqsgen",
    help="A simple, fast and efficient tool to find optimized Special-Quasirandom-Structures (SQS)",
)
@click.version_option(
    prog_name="sqsgen", version=_version_string(), message=print_version()
)
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
def cli(_input, log: str):
    print("CLI", _input)
    config = parse_config(_input.read())
    with click.progressbar(length=config.iterations, label="Optimizing") as bar:
        iterations_finished = 0
        stop_gracefully: bool = False

        def _callback(ctx):
            nonlocal iterations_finished, stop_gracefully
            if stop_gracefully:
                print("Stop signal received...")
                ctx.stop()
            if (actual := ctx.statistics.finished) > iterations_finished:
                bar.update(actual - iterations_finished)
                iterations_finished = actual

        t = threading.Thread(
            target=lambda: optimize(
                config, log_level=LogLevel.debug, callback=_callback
            )
        )
        t.start()
        try:
            print(t.is_alive())
            t.join()
        except KeyboardInterrupt:
            print("Stopping gracefully...")
            stop_gracefully = True
        t.join()
        print()


if __name__ == "__main__":
    cli()
