import os
import shutil
import sys
import tempfile
import platform
import subprocess
from pathlib import PurePosixPath, PureWindowsPath, Path


def gather_wheels_into(
    path: PurePosixPath | PureWindowsPath | str, into: Path
) -> Path | None:
    if not into.exists():
        raise NotADirectoryError(into)

    globbing_pattern = os.path.relpath(path / "sqsgenerator-*.whl", Path(os.getcwd()))
    for wheel in into.glob(str(globbing_pattern)):
        print("Copying: {} -> {}".format(wheel, into))
        shutil.copy(wheel, into)


if __name__ == "__main__":
    match platform.system():
        case "Darwin":
            tmp_dir = PurePosixPath(tempfile.gettempdir())
            built_wheel_dir = tmp_dir / "cibw-run-*" / "cp3*macosx_*" / "built_wheel"
        case "Linux":
            temp_dir = PurePosixPath("/tmp/cibuildwheel")
            built_wheel_dir = temp_dir / "built_wheel"
        case "Windows":
            tmp_dir = PureWindowsPath(r"C:\Users\runneradmin\AppData\Local\Temp")
            built_wheel_dir = (
                tmp_dir / "cibw-run-*" / "cp3*-win_*" / "built_wheel" / ".tmp-*"
            )
        case _:
            raise RuntimeError(f"Unsupported platform: {platform.system()}")

    with tempfile.TemporaryDirectory(prefix="wheel-install") as tmp_dir:
        gather_wheels_into(built_wheel_dir, Path(tmp_dir))
        command_args = [
            sys.executable,
            "-mpip",
            "install",
            "-v",
            str((Path(tmp_dir) / "sqsgenerator*.whl").resolve()),
        ]
        print("Installing wheel with command:", " ".join(command_args))
        subprocess.run(command_args)
