import platform
import subprocess
import sys
import tempfile
from pathlib import PurePosixPath, PureWindowsPath

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

    subprocess.run(
        [
            sys.executable,
            "-mpip",
            "install",
            "-v",
            built_wheel_dir / "sqsgenerator*.whl",
        ]
    )
