import click


def format_hyperlink(text, link: str) -> str:
    return f"\033]8;;{link}\007{text}\033]8;;\007"


def render_error(message: str, parameter: str | None = None) -> None:
    click.echo(
        click.style("Error:", fg="red", bold=True, underline=True) + " " + message
    )
    if parameter is not None:
        link = f"https://sqsgenerator.readthedocs.io/en/latest/input_parameters.html#{parameter}"
        click.echo(
            click.style("Help:", fg="blue", bold=True)
            + f' the documentation for parameter "{parameter}" is available at: {format_hyperlink(link, link)}'
        )
