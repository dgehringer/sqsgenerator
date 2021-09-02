
# Install `sqsgenerator` ...

### Prerequisites
 
`sqsgenerator` is Python 3 CLI app. However, its core is written in C++, thus some prerequisites are needed. Please read
through this section carefully.

  - A C++ compiler, such as `g++`, which has support for [OpenMP](https://www.openmp.org/) and supports C++17
  - [Boost](https://www.boost.org/) libraries are needed to compile the core modules. In particular the following subset of the boost libraries are needed
    - `libboost_python` (Python bindings)
    - `libboost_numpy` (Python bindings)
    - `libboost_log` (logging)
    - `libboost_log_setup` (logging)
    - [Boost](https://www.boost.org/) is used to create the Python bindings, logging, multiprecision math and data structures.
  - [CMake](https://cmake.org/) is used as a build system. A version greater than 3.12 is needed
  - MPI libraries, in case you want to build a MPI enabled version of `sqsgenerator`

Please make sure to have **all** those tools installed, before installing sqsgenerator.

```{note}
   [`ase`](https://wiki.fysik.dtu.dk/ase/) and [`pymatgen`](https://pymatgen.org/) are not explictly listed as
   dependencies of `sqsgenerator`. However ẁe **strongly encourage** you to install at least one of them.
   `sqsgenerator` uses those packages as backends to export the its results into different file formats. Without
   `ase` or `pymatgen` results can be exported in YAML format
```

## ... with `conda`

`sqsgenerator` is available on the `conda-forge` channel. In case you are already using an Anaconda distribution we 
strongly encourage you to head for this installation option

```{code-block} bash
conda install -c conda-forge sqsgenerator
```

To allow for fast generation of random structures, `sqsgenerator`s core modules support hybrid parallelization ([OpenMP](https://www.openmp.org) + [MPI](https://www.mpi-forum.org/))
The version hosted on `conda-forge` supports thread-level-parallelism (only [OpenMP](https://www.openmp.org))

```{note}
To obtain an **MPI enabled** version of the package, you have to **build it** yourself.
```


## ... with `pip`

`sqsgenerator` is not hosted on PyPi. You still can install the package using `pip` by entering the following lines 
tino your terminal:

```{code-block} bash
pip install git+https://github.com/dgehringer/sqsgenerator
# for a MPI enabled build
pip install git+https://github.com/dgehringer/sqsgenerator --install-option="--with-mpi"
```

```{note} 
`sqsgenerator`'s core modules are (for the sake of computational efficiency) written in C++. 
To compile the extenstions you need the following prerequisites:

  - CMake
  - g++
  - boost
  - (MPI libraries)
  
 Please make sure to install them before you install the package
```

## ... by building it yourself

### Environment variable forwarding

`sqsgenerator`'s `setup.py` forwards all environment variables with a `SQS_` prefix to **cmake**.
This allows you to create the binaries with your own configuration.
E. g. [FindBoost.cmake](https://cmake.org/cmake/help/latest/module/FindBoost.html#hints) takes a `BOOST_ROOT` hint to allow
the use of a custom boost version. Therefore to build `sqsgenerator` with your own Boost version use 


   ```{code-block} bash
   SQS_BOOST_ROOT=/path/to/own/boost python setup.py install # --with-mpi
   ```

The above code will under-the-hood call **cmake** with a `-DBOOST_ROOT=/path/to/own/boost` option.

In the same manner values can be passed to [FindMPI.cmake](https://cmake.org/cmake/help/latest/module/FindMPI.html).

### with `conda` on Linux (or  Mac)

1. **Create** an anaconda environment using

    ```{code-block} bash
    conda create --name sqs-build python=3
    ```

2. **Install** required dependencies dependencies (CMake, g++, boost)
    ```{code-block} bash
    conda activate sqs-build
    conda install -c anaconda boost
    # in case you do not have a system g++/cmake
    conda install -c anaconda cmake gxx_linux-64 
    ```
    in case you want to have an MPI enabled build also install the `openmpi` package
    ```{code-block} bash
    conda install -c conda-forge openmpi
    ```

3. **Download** the sources

   ```{code-block} bash
   git clone https://github.com/dgehringer/sqsgenerator.git
   ```

4. **Build & install** the package<br>
    
    To ensure that we use Anaconda's `boost` libs (in case also system libraries are installed) we have to give 
    **cmake** a hint, where to find them. The same is true if you want to use Anacondas C++ compiler (`CMAKE_CXX_COMPILER="x86_64-conda-linux-gnu-g++"`)

    ```{code-block} bash
    SQS_Boost_USE_STATIC_LIBS=ON \
    SQS_Boost_INCLUDE_DIR="${CONDA_PREFIX}/include" \
    SQS_Boost_LIBRARY_DIR_RELEASE="${CONDA_PREFIX}/lib" \
    CMAKE_CXX_COMPILER="x86_64-conda-linux-gnu-g++" \
    python setup.py install
    # or for a MPI build
    python setup.py install --with-mpi
    ```

### performing a custom build

Before performing a custom build make sure you do have a **Python 3** Interpreter as well as 
[`numpy`](https://numpy.org/) installed.
After download the sources and building you can configure **cmake** by setting environment variables.




