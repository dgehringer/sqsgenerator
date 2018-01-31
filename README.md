# sqsgenerator

This package is a Special Quasirandom Structure generator written in Python3/Cython. Please note that the programm currently only works with Python 3.


## Getting Started

To install the package please follow the instructions below. If those steps do not work, feel free to write me a [mail](mailto:dominik-franz-josef.noeger@stud.unileoben.ac.at) with your problem. The package was tested on Ubuntu, Debian and OS X 10.11.

### Prerequisites
The package needs some Python packages to handle input files and overcome Pythons speed limitations. Also the  [GNU GMP](https://gmplib.org/) library is neccesary to make the package run. For parallelization `OpenMP` is used, thus a compiler implementing the OpenMP standard (`gcc >= 4.2`) is needed.

**Attention:** On OS X there is by default no OpenMP enabled compiler since. Use [brew](https://brew.sh/index.html) or [MacPorts](https://www.macports.org) to install a suitable compiler (`brew install gcc` or `sudo ports install gcc**`).

#### Python packages
 Please install [`cython`](https://github.com/cython/cython) and [`pymatgen`](https://github.com/materialsproject/pymatgen).

```
pip install cython 
pip install pymatgen
```

#### GNU gmp library
##### Linux
Please install the `gmp` library and the corresponding __header files__ on your system with your package manager.
On Debian based systems please install the corresponding gmp packages by installing `libgmp10` and `libgmp-dev`.

##### Mac OS X
The most simple way to install `gmp` on OS X is using homebrew

```
brew install gmp
```



### Install procedure

##### Linux
On Linux if you have installed `gmp` using a package manager and not by yourself, setup should work out of the box.

##### Mac OS X or custom `gmp` or custom compiler
You can specify a custom compiler and a custom build of `gmp`. If you are using OS X you have to go through this steps.

1. Open [setup.py](https://github.com/dnoeger/sqsgenerator/blob/master/setup.py) with an editor
2. Uncomment line: `os.environ['CC'] =`
3. Edit the following lines as given in the example below

```
#Specify OpenMP capable compiler
os.environ['CC'] = 'compiler-executable'

#Directory of gmp.h
INCLUDE_DIRECTORIES = ['path/to/your/includes']

#Specify gmp lib directory
LIB_DIRS = ['path/to/your/libraries']
```

If you are using `brew` the aforementioned lines may look like the following

```
#Specify OpenMP capable compiler
os.environ['CC'] = 'gcc-7'

#Directory of gmp.h
INCLUDE_DIRECTORIES = ['/usr/local/include']

#Specify gmp lib directory
LIB_DIRS = ['/usr/local/lib']
```

To install the package run [setup.py](https://github.com/dnoeger/sqsgenerator/blob/master/setup.py)

```
python setup.py install
```

## Usage
