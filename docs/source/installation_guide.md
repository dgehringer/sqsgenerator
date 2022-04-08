
# Install *sqsgenerator* ...

(installation_guide)=

## Optional dependencies - *ase* and *pymatgen*
(optional-dependencies)=

[*ase*](https://wiki.fysik.dtu.dk/ase/) and [*pymatgen*](https://pymatgen.org/) are not explicitly listed as
dependencies of *sqsgenerator*. However, we **strongly encourage** you to install at least one of them.
*sqsgenerator* uses those packages as backends to export its results into different file formats. Without
*ase* or *pymatgen* results can be exported in YAML format only.

## ... with *conda*

*sqsgenerator* is available on the *conda-forge* channel. In case you are already using an Anaconda distribution we 
strongly encourage you to head for this installation option

```{code-block} bash
conda install -c conda-forge sqsgenerator
```

To allow for fast generation of random structures, *sqsgenerator*'s core modules support hybrid parallelization ([OpenMP](https://www.openmp.org) + [MPI](https://www.mpi-forum.org/))
The version hosted on *conda-forge* supports thread-level-parallelism only. ([OpenMP](https://www.openmp.org))

```{note}
To obtain an **MPI enabled** version of the package, you have to **build it** yourself.
```


## ... by building it yourself

### Toolchain 
*sqsgenerator* is Python 3 CLI app. However, its core is written in C++, thus some prerequisites are needed. Please read
through this section carefully.

  - Python interpreter and headers (**>= 3.6**)
  - [numpy](https://numpy.org) installed
  - C++ compiler, with support for [OpenMP](https://www.openmp.org/) and C++17 standard
  - [Boost](https://www.boost.org/) libraries are needed to compile the core modules. In particular the following subset of the boost libraries are needed
    - *libboost_python* (Python bindings)
    - *libboost_numpy* (Python bindings)
    - *libboost_log* (logging)
    - *libboost_log_setup* (logging)
    - *libboost_system* (math, data structures)
    - *libboost_regex*
    - *libboost_thread*
    - [Boost](https://www.boost.org/) is used to create the Python bindings, logging, multiprecision math and data structures.
  - [CMake](https://cmake.org/) is used as a build system. A version greater than 3.12 is needed
  - MPI libraries, in case you want to build the MPI enabled version of *sqsgenerator*

Please make sure to have **all** those tools installed, before installing *sqsgenerator*.

```{admonition} Compatibility between Python interpreter and boost libraries
:class: note

Please make sure that you boost libs *libboost_python* and *libboost_numpy* are compatible with the 
Python interpreter you build *sqsgenerator* against.
```

### Environment variable forwarding

*sqsgenerator*'s *setup.py* forwards all environment variables with a `SQS_` prefix to *CMake*.
This allows you to create the binaries with your own configuration. All variables of form `SQS_{VARNAME}={VALUE}` will
be forwarded to *CMake* as `-D{VARNAME}={VALUE}` via the *setup.py* script.

E. g. [FindBoost.cmake](https://cmake.org/cmake/help/latest/module/FindBoost.html#hints) takes a `BOOST_ROOT` hint to allow
the use of a custom boost version. Therefore, to build *sqsgenerator* with your own Boost version use 


   ```{code-block} bash
   SQS_BOOST_ROOT=/path/to/own/boost pip install # add SQS_USE_MPI=ON for MPI enabled build
   ```

The above code will under-the-hood call *CMake* with a `-DBOOST_ROOT=/path/to/own/boost` option.

In the same manner values can be passed to [FindMPI.cmake](https://cmake.org/cmake/help/latest/module/FindMPI.html).

### with *conda* package manager

#### on Linux or MacOS

With *conda* it is easy to install the needed toolchain, and thus get to a quick customized build

1. **Create** an anaconda environment using

    on Linux use the following command to install the toolchain

    ```{code-block} bash
    conda create --name sqsgen -c conda-forge boost boost-cpp cmake gxx_linux-64 libgomp numpy pip python=3
    ```
   
    on MacOS use this slightly modified version

    ```{code-block} bash
    conda create --name sqsgen -c conda-forge boost boost-cpp cmake llvm-openmp numpy pip python=3
    ```

3. **Download** the sources

   ```{code-block} bash
   git clone https://github.com/dgehringer/sqsgenerator.git
   ```

4. **Build & install** the package<br>
    
    To ensure that we use Anaconda's *boost* libs (in case also system libraries are installed) we have to give 
    *CMake* a hint, where to find them. The same is true if you want to use Anacondas C++ compiler (`CMAKE_CXX_COMPILER="x86_64-conda-linux-gnu-g++"`)

    ```{code-block} bash
    conda activate sqsgen
    cd sqsgenerator
    SQS_Boost_INCLUDE_DIR="${CONDA_PREFIX}/include" \
    SQS_Boost_LIBRARY_DIR_RELEASE="${CONDA_PREFIX}/lib" \
    CMAKE_CXX_COMPILER="x86_64-conda-linux-gnu-g++" \
    CMAKE_CXX_FLAGS="-DNDEBUG -O2 -mtune=native -march=native" \ # optional
    pip install .
    ```
    
    Once you're building *sqsgenerator* yourself it is advisable to optimize it for you machine, which is the reason
    for explicitly specifying compiler optimization flags via `CMAKE_CXX_FLAGS`

    In case you want to build a MPI build version you have to add `SQS_USE_MPI=ON` to the installation instructions.
    In case you also want to link it against a specify MPI implementation (e. g. on a HPC cluster) you can instruct
    *CMake* to so, by adding `SQS_MPI_HOME` which points the installation directory 

    ```{code-block} bash
    cd sqsgenerator
    # SQS_MPI_HOME=/path/to/mpi/implementation/root
    SQS_USE_MPI=ON \
    SQS_Boost_INCLUDE_DIR="${CONDA_PREFIX}/include" \
    SQS_Boost_LIBRARY_DIR_RELEASE="${CONDA_PREFIX}/lib" \
    pip install .
    ```

    On UNIX* systems boost might be installed already by the system package manager. Hence, `SQS_Boost_INCLUDE_DIR`
    and `SQS_Boost_LIBRARY_DIR_RELEASE` point *CMake* to the installation from the conda-environment. For the same
    reason as *g++* is often available, we specify `CMAKE_CXX_COMPILER` explicitly to use the toolchain in the anaconda
    environment.

#### on Windows

Also on Windows, *conda* can be used to install the toolchain. However, the packages shipped in the *conda-forge* 
are built with MSVC compiler toolchain.

```{admonition} MSVC compiler
:class: note

If you use *conda* to install the required libraries, you **need to** have MSVC compiler toolschain installed.
*conda-build* used MSVC to build the libraries, hence MinGW compiler wont't do the job

```

1. **Create** the *conda* environment with and install the toolchain packages

    ```{code-block} bash
   conda create --name sqs-build -c conda-forge boost boost-cpp cmake ninja numpy python=3 
   ```
   
2. **Download** the sources

   ```{code-block} bash
   git clone https://github.com/dgehringer/sqsgenerator.git
   ```

3. **Build & install** the package<br>

    Use the following command to install the package

    ```{code-block} bash
    conda activate sqs-build
    cd sqsgenerator
    CMAKE_CXX_FLAGS="/O2 /DNDEBUG /MD /Zc:twoPhase- /DBOOST_PYTHON_STATIC_LIB /DBOOST_NUMPY_STATIC_LIB /DHAVE_SNPRINTF" pip install .
    ```
   
    - `/MD`: is needed to link against runtime library and boost libraries
    - `/DBOOST_PYTHON_STATIC_LIB /DBOOST_NUMPY_STATIC_LIB`: *libboost_python* and *libboost_numpy* require static linking on Windows
    - `/DHAVE_SNPRINTF`: Keeps compatibility to older MSVC versions. **Note:** Remove this flag if you are using *Visual C++ 14.0* or older