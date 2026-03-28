import io
import json
import os
import re
from subprocess import Popen, PIPE
from datetime import datetime, timedelta, timezone
from pydantic import BaseModel, Field

MAX_AGE = timedelta(days=30)


class GithubWorkflowRun(BaseModel):
    id: int = Field(alias="databaseId")
    created: datetime = Field(alias="createdAt")
    status: str


def fetch_old_runs(
    max_age: timedelta = MAX_AGE, limit: int = 100
) -> list[GithubWorkflowRun]:
    ansi_escape = re.compile(r"(?:\x1B[@-_]|[\x80-\x9F])[0-?]*[ -/]*[@-~]")
    process = Popen(
        [
            "gh",
            "run",
            "list",
            "--limit",
            str(limit),
            "--json",
            "databaseId,createdAt,status",
        ],
        env=dict(NO_COLOR="1", GH_PROMPT_DISABLED="1", **os.environ),
        stdout=PIPE,
    )
    out = io.StringIO()
    for line in process.stdout:
        out.write(ansi_escape.sub("", line.decode()).replace("\r", ""))
    process.wait()
    return [
        gh_run
        for run in json.loads(out.getvalue())
        if (gh_run := GithubWorkflowRun.model_validate(run)).status == "completed"
        and datetime.now(tz=timezone.utc) - gh_run.created > max_age
    ]


def delete_run(run_id: int) -> None:
    process = Popen(
        ["gh", "run", "delete", str(run_id)],
    )
    process.wait()


for run in fetch_old_runs():
    delete_run(run.id)
