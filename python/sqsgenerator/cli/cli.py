import io
import threading

import click

from ..core import LogLevel, optimize, parse_config
from .run import run_optimization
from .version import print_version, version_string


@click.command(
    name="sqsgen",
    help="A simple, fast and efficient tool to find optimized Special-Quasirandom-Structures (SQS)",
)
@click.version_option(
    prog_name="sqsgen", version=version_string(), message=print_version()
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

    run_optimization(_input.read(), log_level=log_level)
    raise click.Abort

    config = parse_config(_input.read())
    with click.progressbar(length=config.iterations, label="Optimizing") as bar:
        iterations_finished = 0
        stop_gracefully: bool = False
        stop_event = threading.Event()

        def _callback(ctx):
            nonlocal iterations_finished, stop_gracefully
            print(stop_gracefully)
            if stop_gracefully:
                print("Stop signal received...")
                ctx.stop()
            if (actual := ctx.statistics.finished) > iterations_finished:
                bar.update(actual - iterations_finished)
                iterations_finished = actual

        def _optimize():
            result = optimize(config, log_level=log_level, callback=_callback)
            print("Optimization finished", result)
            stop_event.set()

        t = threading.Thread(target=_optimize)
        t.start()
        try:
            while t.is_alive() and not stop_event.is_set():
                stop_event.wait()
        except KeyboardInterrupt:
            print("Stopping gracefully...")
            stop_gracefully = True

        t.join()
        print()


if __name__ == "__main__":
    cli()
