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


os.environ["CMAKE_ARGS"] = (
    "-DCMAKE_AR=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/bin/x86_64-conda-linux-gnu-ar -DCMAKE_CXX_COMPILER_AR=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/bin/x86_64-conda-linux-gnu-gcc-ar -DCMAKE_C_COMPILER_AR=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/bin/x86_64-conda-linux-gnu-gcc-ar -DCMAKE_RANLIB=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/bin/x86_64-conda-linux-gnu-ranlib -DCMAKE_CXX_COMPILER_RANLIB=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/bin/x86_64-conda-linux-gnu-gcc-ranlib -DCMAKE_C_COMPILER_RANLIB=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/bin/x86_64-conda-linux-gnu-gcc-ranlib -DCMAKE_LINKER=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/bin/x86_64-conda-linux-gnu-ld -DCMAKE_STRIP=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/bin/x86_64-conda-linux-gnu-strip -DCMAKE_BUILD_TYPE=Release -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -DCMAKE_FIND_ROOT_PATH=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_h_env_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehol;/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/x86_64-conda-linux-gnu/sysroot -DCMAKE_INSTALL_PREFIX=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_h_env_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehol -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_PROGRAM_PATH=/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_build_env/bin;/home/conda/feedstock_root/build_artifacts/sqsgenerator_1753546010825/_h_env_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehol/bin"
)


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
