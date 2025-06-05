import json
import os
import re
import sys
import shlex
import sysconfig
import uuid

import pybind11
import subprocess
from pathlib import Path

from setuptools import Extension, find_packages, setup, findall
from setuptools.command.build_ext import build_ext
from setuptools.command.build_py import build_py as _build_py


# Convert distutils Windows platform specifiers to CMake -A arguments
PLAT_TO_CMAKE = {
    "win32": "Win32",
    "win-amd64": "x64",
    "win-arm32": "ARM",
    "win-arm64": "ARM64",
}

VERSION_FILE = "vcpkg.json"


def make_version_info(path: str) -> dict[str, int]:
    with open(path) as version_file:
        major, minor, build = (
            json.load(version_file).get("version-string", "0.0.0").split(".")
        )

    this_dir = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(this_dir, "version"), "w") as version_file:
        version_file.write(f"{major}.{minor}.{build}\n")

    return dict(major=int(major), minor=int(minor), build=int(build))


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
    def __init__(
        self,
        name: str,
        sourcedir: str = "",
        output_dir: list[str] | None = None,
        build_benchmarks: bool = False,
        build_tests: bool = False,
        build_type: str = "Release",
        version_file: tuple[str, ...] = (VERSION_FILE,),
        vcpkg_root: str | None = None,
    ) -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())
        self.build_type = build_type
        self.build_benchmarks = build_benchmarks
        self.build_tests = build_tests
        self.output_dir = output_dir or []
        self.version_info = make_version_info(
            os.path.join(self.sourcedir, *version_file)
        )
        self.build_dir = uuid.uuid4().hex
        self.vcpkg_root = os.fspath(
            Path(vcpkg_root or os.environ["VCPKG_ROOT"]).resolve()
        )


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        # Must be in this form due to bug in .resolve() only fixed in Python 3.10+
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()

        print(extdir)

        if ext.output_dir:
            extdir = os.path.join(extdir, *ext.output_dir)

        # Using this requires trailing slash for auto-detection & inclusion of
        # auxiliary "native" libs

        if ext.build_type not in {"Release", "Debug", "MinSizeRel", "RelWithDebInfo"}:
            raise ValueError(f"Invalid build type {ext.build_type}")

        cfg = ext.build_type

        # CMake lets you override the generator - we need to check this.
        # Can be set with Conda-Build, for example.
        cmake_generator = os.environ.get("CMAKE_GENERATOR", "")

        # Set Python_EXECUTABLE instead if you use PYBIND11_FINDPYTHON
        # EXAMPLE_VERSION_INFO shows you how to pass a value into the C++ code
        # from Python.
        vcpkg_toolchain = os.path.join(
            ext.vcpkg_root, "scripts", "buildsystems", "vcpkg.cmake"
        )
        include_dir = sysconfig.get_path("include")
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            f"-DPython3_EXECUTABLE={sys.executable}",
            f"-DPython3_INCLUDE_DIR={include_dir}",
            f"-DPython3_LIBRARY={sysconfig.get_config_var('LIBDIR')}",
            f"-DCMAKE_BUILD_TYPE={cfg}",  # not used on MSVC, but no harm
            f"-DBUILD_TESTS={'ON' if ext.build_tests else 'OFF'}",
            f"-DBUILD_BENCHMARKS={'ON' if ext.build_benchmarks else 'OFF'}",
            "-DWITH_MPI=OFF",
            "-DBUILD_PYTHON=ON",
            f"-DCMAKE_TOOLCHAIN_FILE={vcpkg_toolchain}",
            f"-DCMAKE_PREFIX_PATH={pybind11.get_cmake_dir()}",
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

        subprocess.run(
            ["cmake", ext.sourcedir, *cmake_args],
            cwd=build_temp,
            check=True,
            stdout=sys.stdout,
            stderr=sys.stderr,
        )
        subprocess.run(
            ["cmake", "--build", ".", *build_args],
            cwd=build_temp,
            check=True,
            stdout=sys.stdout,
            stderr=sys.stderr,
        )

        subprocess.run(["stubgen", "-m", "_core", "-o", "."], check=False, cwd=extdir)


# The information here can also be placed in setup.cfg - better separation of
# logic and declaration, and simpler if you include description/version in a file.
setup(
    ext_modules=[
        CMakeExtension(
            "sqsgenerator.core._core",
            sourcedir="..",
            vcpkg_root=os.path.join("..", "vcpkg"),
        )
    ],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    packages=find_packages(".", exclude=["tests"]),
)
