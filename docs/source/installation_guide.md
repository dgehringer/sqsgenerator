
# Install `sqsgenerator` ...

## Optional dependencies - `ase` and `pymatgen`

[`ase`](https://wiki.fysik.dtu.dk/ase/) and [`pymatgen`](https://pymatgen.org/) are not explicitly listed as
dependencies of `sqsgenerator`. However we **strongly encourage** you to install at least one of them.
`sqsgenerator` uses those packages as backends to export the its results into different file formats. Without
`ase` or `pymatgen` results can be exported in YAML format only.

## ... with `conda`

```{warning}
`sqsgenerator` is still under evaluation, therefore the newest version is **yet** not available at the conda-forge channel
Until evaluation phase is finished, we kindly ask you to **build it yourself**, or to use one of the **pre-built binaries**.

```

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

`sqsgenerator` is not hosted on PyPi. You still can install the package using `pip` by using one of the 
prebuilt binary packages:


| OS    | arch   | Py   | Wheel                                                        |
| ----- | ------ | ---- | ------------------------------------------------------------ |
| linux | x86_64 | 3.6  | [sqsgenerator-0.1-cp36-cp36m-linux_x86_64.whl](http://oc.unileoben.ac.at/index.php/s/qqDIydH02PkV32V/download) |
| linux | x86_64 | 3.7  | [sqsgenerator-0.1-cp37-cp37m-linux_x86_64.whl](http://oc.unileoben.ac.at/index.php/s/34xlYhyZxkyb6xy/download) |
| linux | x86_64 | 3.8  | [sqsgenerator-0.1-cp38-cp38-linux_x86_64.whl](http://oc.unileoben.ac.at/index.php/s/gTk345lGTwk3C0G/download) |
| linux | x86_64 | 3.9  | [sqsgenerator-0.1-cp39-cp39-linux_x86_64.whl](http://oc.unileoben.ac.at/index.php/s/3x01KBKarx11BgQ/download) |

Once you have choosen the correct wheel, you can easily install it:

```{code-block} bash
pip install sqsgenerator-0.1-cp36-cp36m-linux_x86_64.whl
```


## ... by building it yourself

### Toolchain 
`sqsgenerator` is Python 3 CLI app. However, its core is written in C++, thus some prerequisites are needed. Please read
through this section carefully.

  - Python interpreter and headers (**>= 3.6**)
  - [numpy](https://numpy.org) installed
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

### on Linux (or  Mac)

### with `conda` package manager

With `conda` it is easy to install the needed toolchain, and thus get to a quick customized build

1. **Create** an anaconda environment using

    ```{code-block} bash
    conda create --name sqs-build python=3
    ```

2. **Install** required dependencies dependencies (CMake, g++, boost)
 
    with `conda` it is easy to install the toolchain needed to make the binaries    

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

5. **Remove build dependencies**

    This step is only optional. In case you have compiled the core modules with `SQS_Boost_USE_STATIC_LIBS=ON` the created
    library does not depend any more on the `boost` libraries.
    In case you **do not need** them any more and you can remove them again.

