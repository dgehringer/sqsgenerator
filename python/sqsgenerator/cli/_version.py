import io
from typing import Any, Optional

import click

from ..core import __build__, __version__
from ._shared import format_hyperlink


def version_string() -> str:
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

    def print_row(name: str, value: str, hyperlink: Optional[str] = None) -> None:
        write(f"{name}:", bold=True, fg="green")
        write(
            f" {value if hyperlink is None else format_hyperlink(value, hyperlink)}\n"
        )

    print_row("Version", version_string())
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
