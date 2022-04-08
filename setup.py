
import re
import os
import sys
import pprint
import platform
import subprocess
from setuptools.command.install import install
from setuptools.command.build_ext import build_ext
from setuptools import setup, Extension, find_packages

WITH_MPI = False


class CMakeExtension(Extension):
    def __init__(self, name, target='', cmake_lists_dir='.', debug=False, verbose=True, **kwargs):
        Extension.__init__(self, name, sources=[], **kwargs)
        self.cmake_lists_dir = os.path.abspath(cmake_lists_dir)
        self.debug = debug
        self.target = target
        self.verbose = verbose


class InstallCustom(install):
    user_options = install.user_options + [
        ('with-mpi', None, 'Enables MPI Parallelization')
    ]

    def initialize_options(self):
        super(InstallCustom, self).initialize_options()
        self.with_mpi = WITH_MPI

    def finalize_options(self):
        super(InstallCustom, self).finalize_options()
        global WITH_MPI
        WITH_MPI = self.with_mpi


class CMakeBuildExt(build_ext):
    user_options = build_ext.user_options + [
        ('with-mpi', None, 'Enables MPI Parallelization')
    ]

    def build_extensions(self):
        # Ensure that CMake is present and working
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError('Cannot find CMake executable')

        os.makedirs(self.build_temp, exist_ok=True)

        for ext in self.extensions:
            extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
            cfg = 'Debug' if ext.debug else 'Release'
            cmake_args = [
                f'-DCMAKE_BUILD_TYPE={cfg}',
                # Ask CMake to place the resulting library in the directory
                # containing the extension
                f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}',
                # Other intermediate static libraries are placed in a
                # temporary build directory instead
                f'-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_{cfg.upper()}={self.build_temp}',
                # Hint CMake to use the same Python executable that
                # is launching the build, prevents possible mismatching if
                # multiple versions of Python are installed
                f'-DPython3_EXECUTABLE={sys.executable}',
                # Add other project-specific CMake arguments if needed
                # ...
                '-DUSE_MPI={}'.format("ON" if self.with_mpi else "OFF"),
                # '-DCMAKE_CXX_FLAGS_{}={}'.format(cfg.upper(),  )
            ]
            cmake_cxx_flags = ''

            env_var_prefix = 'SQS_'
            # we allow overloading cmake compiler options
            # for windows builds CMake chooses bad default on azure-pipelines
            # we implement it as a whitelist
            whitelist_env_var_name = 'SQS_FORWARD_WHITELIST'
            cmake_var_whitelist = os.environ.get(whitelist_env_var_name, None)
            cmake_var_whitelist = cmake_var_whitelist \
                if cmake_var_whitelist is None \
                else list(filter(bool, cmake_var_whitelist.split(',')))

            for env_var_name, env_var_value in os.environ.items():
                if env_var_name == whitelist_env_var_name:
                    continue
                if env_var_name == 'CMAKE_CXX_FLAGS':
                    # we append them to our release/debug flags
                    cmake_cxx_flags += f' {env_var_value}'
                elif env_var_name.startswith('CMAKE'):

                    if cmake_var_whitelist is not None:
                        if env_var_name not in cmake_var_whitelist:
                            print(f'sqsgenerator.setup: Blocking env-var "{env_var_name}" '
                                  f'since it is not whitelisted in {whitelist_env_var_name}')
                            continue
                    print(f'sqsgenerator.setup: Forwarding env-var "{env_var_name}" -> "-D{env_var_name}"')
                    cmake_args.append(f'-D{env_var_name}={env_var_value}')
                m = re.match(f'{env_var_prefix}(?P<varname>\w+)', env_var_name)
                if m:
                    env_var_name_real = m.groupdict()['varname']
                    print(f'sqsgenerator.setup: Forwarding env-var "{env_var_name}" -> "-D{env_var_name_real}"')
                    cmake_args.append(f'-D{env_var_name_real}={env_var_value}')


            # We can handle some platform-specific settings at our discretion
            if platform.system() == 'Windows':
                print('sqsgenerator.setup.py -> configuring CMake for Windows')
                # plat = ('x64' if platform.architecture()[0] == '64bit' else 'Win32')
                cmake_args += [
                    # These options are likely to be needed under Windows
                    # '-DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE',
                    f'-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}'
                ]
                # Assuming that Visual Studio and MinGW are supported compilers
                if self.compiler.compiler_type == 'msvc':
                    cmake_args += [
                        # f'-DCMAKE_GENERATOR_PLATFORM={plat}',
                        '-G', 'Ninja'
                    ]
                else:
                    cmake_args += [
                        '-G', 'MinGW Makefiles',
                    ]

            cmake_args.append(f'-DCMAKE_CXX_FLAGS_{cfg.upper()}={cmake_cxx_flags}')

            # for debugging reasons we print the actual config which is passed to cmake
            pprint.pprint(cmake_args)

            subprocess.check_call(['cmake', ext.cmake_lists_dir] + cmake_args, cwd=self.build_temp)
            cmake_build_args = ['cmake', '--build', '.', '--config', cfg]
            if ext.target: cmake_build_args += ['--target', ext.target]
            if ext.verbose: cmake_build_args += ['--verbose']

            subprocess.check_call(cmake_build_args, cwd=self.build_temp)

    def initialize_options(self):
        super(CMakeBuildExt, self).initialize_options()
        self.with_mpi = WITH_MPI

    def finalize_options(self):
        """finalize options"""
        super(CMakeBuildExt, self).finalize_options()


setup(
    name='sqsgenerator',
    version='0.1',
    description='A simple command line Special Quasirandom Structure generator in Python',
    author_email='dgehringer@protonmail.com',
    license='MIT',
    ext_modules = [
        CMakeExtension('sqsgenerator.core.core', 'core'),
    ],
    cmdclass = {
        'build_ext': CMakeBuildExt,
        'install': InstallCustom
    },
    install_requires=['six', 'numpy', 'click', 'rich>=9.11.0', 'pyyaml', 'frozendict'],
    entry_points={
        'console_scripts': ['sqsgen=sqsgenerator.cli:cli']
    },
    packages=find_packages('.', exclude=['test']),
    include_package_data=True
)
