import os
import sys
import subprocess
import platform
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.install import install

ENABLE_MPI = False




class CMakeExtension(Extension):
    def __init__(self, name, target='', cmake_lists_dir='.', debug=False, verbose=False, **kwargs):
        Extension.__init__(self, name, sources=[], **kwargs)
        self.cmake_lists_dir = os.path.abspath(cmake_lists_dir)
        self.debug = debug
        self.target = target
        self.verbose = verbose


class cmake_build_ext(build_ext):
    user_options = build_ext.user_options + [
        ('with-mpi', None, 'Enables MPI Parallelization')
    ]
    def build_extensions(self):
        # Ensure that CMake is present and working
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError('Cannot find CMake executable')
        cmake_initialized = False

        for ext in self.extensions:

            extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
            cfg = 'Debug' if ext.debug else 'Release'
            opt_args = []
            cmake_args = [
                f'-DCMAKE_BUILD_TYPE={cfg}'
                # Ask CMake to place the resulting library in the directory
                # containing the extension
                f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}'
                # Other intermediate static libraries are placed in a
                # temporary build directory instead
                f'-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_{cfg.upper()}={self.build_temp}',
                # Hint CMake to use the same Python executable that
                # is launching the build, prevents possible mismatching if
                # multiple versions of Python are installed
                # '-DPython3_EXECUTABLE={}'.format(sys.executable),
                # Add other project-specific CMake arguments if needed
                # ...
                f'-DUSE_MPI={"ON" if self.with_mpi else "OFF"}'
            ]


            # We can handle some platform-specific settings at our discretion
            if platform.system() == 'Windows':
                plat = ('x64' if platform.architecture()[0] == '64bit' else 'Win32')
                cmake_args += [
                    # These options are likely to be needed under Windows
                    '-DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE',
                    '-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir),
                ]
                # Assuming that Visual Studio and MinGW are supported compilers
                if self.compiler.compiler_type == 'msvc':
                    cmake_args += [
                        '-DCMAKE_GENERATOR_PLATFORM=%s' % plat,
                    ]
                else:
                    cmake_args += [
                        '-G', 'MinGW Makefiles',
                    ]

            if sys.platform.startswith("darwin"):
                # Cross-compile support for macOS - respect ARCHFLAGS if set
                archs = re.findall(r"-arch (\S+)", os.environ.get("ARCHFLAGS", ""))
                if archs:
                    cmake_args += ["-DCMAKE_OSX_ARCHITECTURES={}".format(";".join(archs))]

            if not os.path.exists(self.build_temp):
                os.makedirs(self.build_temp)

            # Config
            if not cmake_initialized:
                subprocess.check_call(['cmake', ext.cmake_lists_dir] + cmake_args,
                                  cwd=self.build_temp)
                cmake_initialized = True

            cmake_build_args = ['cmake', '--build', '.', '--config', cfg]
            if ext.target: cmake_build_args += ['--target', ext.target]
            if ext.verbose: cmake_build_args += ['--verbose']
            subprocess.check_call(cmake_build_args, cwd=self.build_temp)

    def initialize_options(self):
        super(cmake_build_ext, self).initialize_options()
        self.with_mpi = ENABLE_MPI

    def finalize_options(self):
        """finalize options"""
        super(cmake_build_ext, self).finalize_options()

    #def run(self):
    #    super(cmake_build_ext, self).run()
        
setup(name = "sqsenerator",
      version = "0.1",
      ext_modules = [
          CMakeExtension('sqsgenerator.core.data', 'data'),
          CMakeExtension('sqsgenerator.core.iteration', 'iteration')
      ],
      cmdclass = {'build_ext': cmake_build_ext},
      )