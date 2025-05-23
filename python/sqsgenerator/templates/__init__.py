import functools
import json
import os
from typing import Any


@functools.lru_cache(maxsize=1)
def load_templates() -> dict[str, dict[str, Any]]:
    """Load all templates from the templates directory."""
    templates = {}
    for *_, files in os.walk(this_dir := os.path.join(os.path.dirname(__file__))):
        for filename in files:
            stem, extension = os.path.splitext(filename)
            if extension == ".json":
                with open(os.path.join(this_dir, filename)) as f:
                    template = json.load(f)
                templates[stem] = template
        break
    return templates
