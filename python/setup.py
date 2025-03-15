import json
import logging
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path
from typing import List, Union

from setuptools import Extension, setup, find_packages
from setuptools.command.build_ext import build_ext

# Convert distutils Windows platform specifiers to CMake -A arguments
PLAT_TO_CMAKE = {
    "win32": "Win32",
    "win-amd64": "x64",
    "win-arm32": "ARM",
    "win-arm64": "ARM64",
}

def git_sha1() -> str:
    try:
        return os.popen("git rev-parse HEAD").read().strip()
    except:
        return "unknown"

def git_branch() -> str:
    try:
        return os.popen("git rev-parse --abbrev-ref HEAD").read().strip()
    except:
        return "unknown"


# A CMakeExtension needs a sourcedir instead of a file list.
# The name must be the _single_ output extension from the CMake build.
# If you need multiple extensions, see scikit-build.
class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = "", output_dir: List[str] = None, build_benchmarks: bool = False,
                 build_tests: bool = False, build_type: str = "release", version_file: str = "version.json") -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())
        self.build_type = build_type
        self.build_benchmarks = build_benchmarks
        self.build_tests = build_tests
        self.output_dir = output_dir
        version_path = os.path.join(self.sourcedir, version_file)
        if not os.path.exists(version_path):
            raise FileNotFoundError(version_path)
        else:
            with open(version_path, 'rb') as f:
                self.version_info = json.load(f)


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        # Must be in this form due to bug in .resolve() only fixed in Python 3.10+
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()

        if ext.output_dir:
            extdir = os.path.join(extdir, *ext.output_dir)

        # Using this requires trailing slash for auto-detection & inclusion of
        # auxiliary "native" libs

        if ext.build_type not in {"release", "debug"}:
            raise ValueError(f"Invalid build type {ext.build_type}")

        cfg = ext.build_type

        # CMake lets you override the generator - we need to check this.
        # Can be set with Conda-Build, for example.
        cmake_generator = os.environ.get("CMAKE_GENERATOR", "")

        # Set Python_EXECUTABLE instead if you use PYBIND11_FINDPYTHON
        # EXAMPLE_VERSION_INFO shows you how to pass a value into the C++ code
        # from Python.
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            f"-DPython3_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE={cfg}",  # not used on MSVC, but no harm
            f"-DBUILD_TESTS={'ON' if ext.build_tests else 'OFF'}",
            f"-DBUILD_BENCHMARKS={'ON' if ext.build_benchmarks else 'OFF'}",
            f"-DWITH_MPI=OFF",
            f"-DBUILD_PYTHON=ON",
            f"-DSQSGEN_MAJOR_VERSION={ext.version_info['major']}",
            f"-DSQSGEN_MINOR_VERSION={ext.version_info['minor']}",
            f"-DSQSGEN_BUILD_NUMBER={ext.version_info['build']}",
            f"-DSQSGEN_BUILD_BRANCH={git_branch()}",
            f"-DSQSGEN_BUILD_COMMIT={git_sha1()}",
        ]
        build_args = []
        # Adding CMake arguments set as environment variable
        # (needed e.g. to build for ARM OSx on conda-forge)
        if "CMAKE_ARGS" in os.environ:
            cmake_args += [item for item in os.environ["CMAKE_ARGS"].split(" ") if item]

        if self.compiler.compiler_type != "msvc":
            # Using Ninja-build since it a) is available as a wheel and b)
            # multithreads automatically. MSVC would require all variables be
            # exported for Ninja to pick it up, which is a little tricky to do.
            # Users can override the generator with CMAKE_GENERATOR in CMake
            # 3.15+.
            if not cmake_generator or cmake_generator == "Ninja":
                try:
                    import ninja

                    ninja_executable_path = Path(ninja.BIN_DIR) / "ninja"
                    cmake_args += [
                        "-GNinja",
                        f"-DCMAKE_MAKE_PROGRAM:FILEPATH={ninja_executable_path}",
                    ]
                except ImportError:
                    pass

        else:
            # Single config generators are handled "normally"
            single_config = any(x in cmake_generator for x in {"NMake", "Ninja"})

            # CMake allows an arch-in-generator style for backward compatibility
            contains_arch = any(x in cmake_generator for x in {"ARM", "Win64"})

            # Specify the arch if using MSVC generator, but only if it doesn't
            # contain a backward-compatibility arch spec already in the
            # generator name.
            if not single_config and not contains_arch:
                cmake_args += ["-A", PLAT_TO_CMAKE[self.plat_name]]

            # Multi-config generators have a different way to specify configs
            if not single_config:
                cmake_args += [
                    f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}"
                ]
                build_args += ["--config", cfg]

        if sys.platform.startswith("darwin"):
            # Cross-compile support for macOS - respect ARCHFLAGS if set
            archs = re.findall(r"-arch (\S+)", os.environ.get("ARCHFLAGS", ""))
            if archs:
                cmake_args += ["-DCMAKE_OSX_ARCHITECTURES={}".format(";".join(archs))]

        # Set CMAKE_BUILD_PARALLEL_LEVEL to control the parallel build level
        # across all generators.
        if "CMAKE_BUILD_PARALLEL_LEVEL" not in os.environ:
            # self.parallel is a Python 3 only way to set parallel jobs by hand
            # using -j in the build_ext call, not supported by pip or PyPA-build.
            if hasattr(self, "parallel") and self.parallel:
                # CMake 3.12+ only.
                build_args += [f"-j{self.parallel}"]

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)
        print(shlex.join(["cmake", ext.sourcedir, *cmake_args]))
        subprocess.run(
            ["cmake", ext.sourcedir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            ["cmake", "--build", ".", *build_args], cwd=build_temp, check=True
        )


# The information here can also be placed in setup.cfg - better separation of
# logic and declaration, and simpler if you include description/version in a file.
setup(
    name="sqsgenerator",
    version="0.4.1",
    author="Dominik Gehringer",
    author_email="dgehringer@protonmail.com",
    description="Create atomic structures for molecular simulations",
    long_description="",
    ext_modules=[CMakeExtension("_core", sourcedir="..", output_dir=["sqsgenerator", "core"])],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    install_requires=['numpy', 'click', 'rich'],
    extras_require={"test": ["pytest>=6.0"]},
    python_requires=">=3.9",
    packages=find_packages('.', exclude=['tests']),
)
