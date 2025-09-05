import base64
import gzip
import json
import webbrowser
from urllib.parse import urlencode

import click

from .._optimize import _preprocess_structure, parse_config
from ..core import ParseError
from ._shared import format_hyperlink, render_error

_APP_URL = "https://sqsgen.gehringer.tech"


def link(input_json: str, open_in_browser: bool) -> None:
    if isinstance(config := parse_config(input_json), ParseError):
        render_error(config.msg, parameter=config.key)
        return
    input_data = json.loads(input_json)
    if structure_config := input_data.get("structure"):
        _preprocess_structure(structure_config)
        if "file" in structure_config:
            del structure_config["file"]
    params = dict(
        config=base64.b64encode(gzip.compress(json.dumps(input_data).encode())).decode()
    )
    url = f"{_APP_URL}?{urlencode(params)}"
    if open_in_browser:
        webbrowser.open_new_tab(url)
    else:
        click.echo(format_hyperlink(url, url))
