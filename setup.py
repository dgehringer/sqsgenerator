from distutils.core import setup
from setuptools import find_packages
from distutils.extension import Extension
from Cython.Build import cythonize
from os.path import join, dirname, basename, exists
from os import mkdir
from shutil import copytree, rmtree
import numpy
import os

#Specify OpenMP capable compiler
#os.environ['CC'] = '/usr/local/bin/clang-omp'

#Directory of gmp.h
#INCLUDE_DIRECTORIES = ['/usr/local/include']
INCLUDE_DIRECTORIES = []

#Specify gmp lib directory
#LIB_DIRS = ['/usr/local/Cellar/gmp/6.1.2_1/lib', '/usr/local/opt/llvm/lib']
LIB_DIRS = []

################################################
THIS_DIR = dirname(__file__)
CORE_EXT_DIRECTORY = join('sqsgenerator', 'core')
BUILD_DIRECTORY = join(THIS_DIR, CORE_EXT_DIRECTORY)
EXTRA_LINK_ARGS = ['-L{ld}'.format(ld=libdir) for libdir in LIB_DIRS]+['-lgmp']
EXTRA_COMPILE_ARGS = ['-std=c1x']
INCLUDE_DIRS = [numpy.get_include(), join(BUILD_DIRECTORY, 'include')] + INCLUDE_DIRECTORIES

with open(join(THIS_DIR, 'README.md'), encoding='utf-8') as file:
    long_description = file.read()


#if exists(BUILD_DIRECTORY):
#    rmtree(BUILD_DIRECTORY)

#copytree(CORE_EXT_DIRECTORY, join(BUILD_DIRECTORY))
#os.remove(join(BUILD_DIRECTORY, '__init__.py'))


ext_modules = [
    Extension(
        name='sqsgenerator.core.base',
        sources=[join(BUILD_DIRECTORY, 'base.pyx'),
                 join(BUILD_DIRECTORY, 'src', 'utils.c'),
                 join(BUILD_DIRECTORY, 'src', 'rank.c')],
        extra_compile_args=EXTRA_COMPILE_ARGS,
        extra_link_args=EXTRA_LINK_ARGS,
        include_dirs=INCLUDE_DIRS
    ),
    Extension(
        name='sqsgenerator.core.collection',
        sources=[join(BUILD_DIRECTORY, 'collection.pyx'),
                 join(BUILD_DIRECTORY, 'src', 'list.c'),
                 join(BUILD_DIRECTORY, 'src', 'conf_list.c'),
                 join(BUILD_DIRECTORY, 'src', 'conf_array.c'),
                 join(BUILD_DIRECTORY, 'src', 'conf_collection.c'),
                 join(BUILD_DIRECTORY, 'src', 'rank.c'),
                 join(BUILD_DIRECTORY, 'src', 'utils.c')],
        extra_compile_args=EXTRA_COMPILE_ARGS,
        extra_link_args=EXTRA_LINK_ARGS,
        include_dirs=INCLUDE_DIRS
    ),
    Extension(
        name='sqsgenerator.core.sqs',
        sources=[join(BUILD_DIRECTORY, 'sqs.pyx'),
                 join(BUILD_DIRECTORY, 'src', 'utils.c'),
                 join(BUILD_DIRECTORY, 'src', 'rank.c')
                 ],
        extra_compile_args=['-fopenmp'] + EXTRA_COMPILE_ARGS,
        extra_link_args=['-fopenmp'] + EXTRA_LINK_ARGS,
        include_dirs=INCLUDE_DIRS
    ),
Extension(
        name='sqsgenerator.core.dosqs',
        sources=[join(BUILD_DIRECTORY, 'dosqs.pyx'),
                 join(BUILD_DIRECTORY, 'src', 'utils.c'),
                 join(BUILD_DIRECTORY, 'src', 'rank.c')
                 ],
        extra_compile_args=['-fopenmp'] + EXTRA_COMPILE_ARGS,
        extra_link_args=['-fopenmp'] + EXTRA_LINK_ARGS,
        include_dirs=INCLUDE_DIRS
    )
]


setup(
        name='sqsgenerator',
        version="v0.05",
        description='A simple command line Special Quasirandom Structure generator program in Python/Cython.',
        long_description=long_description,
        author='Dominik Gehringer',
        author_email='dominik.gehringer@unileoben.ac.at',
        license='UNLICENSE',
        classifiers=[
            'Intended Audience :: Developers',
            'Topic :: Utilities',
            'License :: Public Domain',
            'Natural Language :: English',
            'Operating System :: OS Independent',
            'Programming Language :: Python :: 3',
            'Programming Language :: Python :: 3.2',
            'Programming Language :: Python :: 3.3',
            'Programming Language :: Python :: 3.4',
        ],
        keywords='cli',
        install_requires=['numpy', 'pymatgen', 'cython'],
        ext_modules=cythonize(ext_modules),
        packages=find_packages(),
        include_dirs=[numpy.get_include()],
        entry_points={
                'console_scripts': [
                    'sqsgenerator=sqsgenerator.cli:main',
                ]
            }
        # cmdclass = {'test': RunTests},
)



