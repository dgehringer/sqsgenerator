import json
import os.path
from typing import Any

import pytest
from click.testing import CliRunner

from sqsgenerator import load_result_pack
from sqsgenerator.__main__ import cli
from sqsgenerator.core import random
from sqsgenerator.templates import load_templates

NUM_RUNS_PER_TEMPLATE = 3
NUM_ITERATIONS = 50000


def fixup_template_iterations(template: dict[str, Any]) -> dict[str, Any]:
    """Ensure that the template has a valid number of iterations."""
    template["iterations"] = NUM_ITERATIONS
    template["iteration_mode"] = "random"
    return template


@pytest.mark.parametrize("name", list(load_templates().keys()))
@pytest.mark.parametrize("run", range(NUM_RUNS_PER_TEMPLATE))
def test_run_templates(name: str, run: int) -> None:
    """Test running the CLI with each template."""
    runner = CliRunner()
    with runner.isolated_filesystem():
        confile_file_name = f"{name}.sqs.json"

        result = runner.invoke(cli, ["template", name])

        assert result.exit_code == 0, (
            f"Template {name} failed on run {run}: {result.output}"
        )
        assert "Template: " in result.output and name in result.output, (
            f"Template {name} did not print correctly on run {run}"
        )

        assert os.path.exists(confile_file_name)

        with open(confile_file_name) as config_file:
            template = json.load(config_file)

        with open(confile_file_name, "w") as config_file:
            json.dump(fixup_template_iterations(template), config_file, indent=2)

        result = runner.invoke(cli, ["run", "-i", confile_file_name, "--quiet"])
        assert result.exit_code == 0
        result_file_name = f"{name}.sqs.mpack"
        assert os.path.exists(result_file_name)
        result_pack = load_result_pack(result_file_name)

        assert result_pack.config.iteration_mode == random
        assert result_pack.config.iterations == NUM_ITERATIONS
