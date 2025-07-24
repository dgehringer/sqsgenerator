import threading
from typing import Optional

import click

from ..core import (
    LogLevel,
    ParseError,
    SqsCallbackContext,
    SqsResultPack,
    optimize,
    parse_config,
)
from ._shared import render_error


class _FakeProgressBar:
    def __init__(self, *args, **kwargs):
        pass

    def __enter__(self):
        return self

    def update(self, *args, **kwargs):
        pass

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass


def run_optimization(
    input_json: str, log_level: LogLevel = LogLevel.warn, quiet: bool = False
) -> None:
    config = parse_config(input_json)
    if isinstance(config, ParseError):
        render_error(config.msg, parameter=config.key)

    progressbar = _FakeProgressBar if quiet else click.progressbar

    with progressbar(
        length=config.iterations, label="Optimizing", fill_char="█", empty_char=" "
    ) as bar:
        iterations_finished = 0
        stop_gracefully: bool = False
        stop_event = threading.Event()

        def _callback(ctx: SqsCallbackContext) -> None:
            nonlocal iterations_finished, stop_gracefully
            if stop_gracefully:
                ctx.stop()
            if (actual := ctx.statistics.finished) > iterations_finished:
                bar.update(actual - iterations_finished)
                iterations_finished = actual

        optimization_result: Optional[SqsResultPack] = None

        def _optimize():
            result_local = optimize(config, log_level=log_level, callback=_callback)
            stop_event.set()
            nonlocal optimization_result
            optimization_result = result_local

        t = threading.Thread(target=_optimize)
        t.start()
        try:
            while t.is_alive() and not stop_event.is_set():
                stop_event.wait()
        except (KeyboardInterrupt, EOFError):
            stop_gracefully = True
        finally:
            t.join()

        return optimization_result
