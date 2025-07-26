import shlex

import toml
import sys
import os
import pprint


def get_path(config: dict, *crumbs: str):
    first, *rest = crumbs
    entry = config.get(first)
    if entry is not None:
        return entry if not rest else get_path(entry, *rest)
    else:
        return None


if __name__ == "__main__":
    self, filename, *_ = sys.argv
    pprint.pprint(os.environ)

    with open(filename) as f:
        pyproject = toml.load(f)

    cmake_args = get_path(pyproject, "tool", "scikit-build", "cmake", "args")

    if isinstance(cmake_args, list) and cmake_args:
        if "CMAKE_ARGS" in os.environ:
            current_args = set(cmake_args)
            for arg in shlex.split(os.environ["CMAKE_ARGS"]):
                cmake_args.append(arg)

    dumped = toml.dumps(pyproject)
    print("==== NEW pyproject.toml ====")
    with open(filename, "w") as f:
        f.write(dumped)
