import io
from collections.abc import Iterable
from typing import Any, Optional

import click


def format_hyperlink(text, link: str) -> str:
    return f"\033]8;;{link}\007{text}\033]8;;\007"


def render_error(
    message: str, parameter: Optional[str] = None, info: Optional[str] = None
) -> None:
    click.echo(
        click.style("Error:", fg="red", bold=True, underline=True) + " " + message
    )
    if info is not None:
        click.echo(
            "       ("
            + click.style("info: ", italic=True, fg="blue")
            + click.style(info, italic=True)
            + ")"
        )
    if parameter is not None:
        link = (
            f"https://sqsgenerator.readthedocs.io/en/latest/parameters.html#{parameter}"
        )
        click.echo(
            click.style("Help:", fg="blue", bold=True)
            + f' the documentation for parameter "{parameter}" is available at: {format_hyperlink(link, link)}'
        )


def render_table(
    rows: Iterable[tuple[Any, ...]],
    *columns: str,
    sep: str = " ",
    header_style: Optional[dict[str, Optional[bool]]] = None,
    buf: Optional[io.StringIO] = None,
    **styles: dict[str, Optional[bool]],
) -> None:
    header_style = header_style or dict(underline=True, bold=True)
    max_widths = [
        max(map(len, (title, *col))) for col, title in zip(zip(*rows), columns)
    ]
    buf = buf or io.StringIO()
    buf.write(
        sep.join(
            click.style(title.ljust(w), **header_style)
            for title, w in zip(columns, max_widths)
        )
        + "\n"
    )
    for row in rows:
        buf.write(
            sep.join(
                click.style(value.ljust(w), **styles.get(title, {}))
                for value, title, w in zip(row, columns, max_widths)
            )
            + "\n"
        )

    click.echo_via_pager(buf.getvalue())
