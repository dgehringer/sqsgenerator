from ..core import LogLevel, ParseError, SqsConfiguration, parse_config
from ._shared import render_error


def run_optimization(input_json: str, log_level: LogLevel = LogLevel.warn) -> None:
    config = parse_config(input_json)
    if isinstance(config, ParseError):
        render_error(config.msg, parameter=config.key)
