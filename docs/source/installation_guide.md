
# Install `sqsgenerator` ...

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
  - (mpi libraries)
  
 Please make sure to install them before you install the package
```

## ... by building it yourself

### with `conda` on Linux (or  Mac)

1. **Create** an anaconda environment using

    ```{code-block} bash
    conda create --name sqs-build python=3
    ```

2. **Install** required dependencies dependencies (CMake, g++, boost)
    ```{code-block} bash
    conda activate sqs-build
    conda install -c anaconda cmake gxx_linux-64 boost
    ```
    in case you want to have an MPI enabled build also install the `openmpi` package
    ```{code-block} bash
    conda install -c conda-forge openmpi
    ```

3. **Download** the sources

4. **Build & install** the package<br>

    ```{code-block} bash
    python setup.py install
    python setup.py install --with-mpi
    ```

    ````{warning}
    It might be that CMake detects your system's compiler. Please check that before buidling (`which g++`).
    You can instruct `setup.py` to overrride CMake  default compiler with `CMAKE_CXX_COMPILER` and use that one of your 
    Anaconda distribution 

    ```{code-block} bash
    CMAKE_CXX_COMPILER="x86_64-conda-linux-gnu-g++" python setup.py install 
    CMAKE_CXX_COMPILER="x86_64-conda-linux-gnu-g++" python setup.py install --with-mpi
    ```
    ````
