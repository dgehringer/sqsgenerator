![conda](https://anaconda.org/conda-forge/sqsgenerator/badges/installer/conda.svg) 
![plats](https://anaconda.org/conda-forge/sqsgenerator/badges/platforms.svg)
![downloads](
https://anaconda.org/conda-forge/sqsgenerator/badges/downloads.svg)
[![Documentation Status](https://readthedocs.org/projects/sqsgenerator/badge/?version=latest)](https://sqsgenerator.readthedocs.io/en/latest/?badge=latest)


<p align="center">
  <img src="https://github.com/dgehringer/sqsgenerator/raw/master/docs/source/logo_large.svg" width="60%" alt="sqsgenerator-logo" />
  <br /><br />
  <a href="https://sqsgenerator.readthedocs.io">📝 docs</a>
</p>

---

**sqsgenerator** is a Python package, which allows you to generate optimised *Special-Quasirandom-Structures* (SQS). Therefore the package use *Warren-Cowley Short-Range-Order* (SRO) parameters to quantify randomness. **sqsgenerator** can be also used to analyse SRO parameters in existing structures. The core routines are written in C++

## Highlights

  - :electric_plug: Easy integration with popular frameworks such as [ase](https://wiki.fysik.dtu.dk/ase/),
    [pymatgen](https://pymatgen.org/) and [pyiron](https://pyiron.org/)
  - :curly_loop: Monte-Carlo and systematic approach to compute optimal atomic configuration
  - :rocket: **Carefully hand-crafted** low-level C++ routines, for efficient calculation of short-range-order
  - :twisted_rightwards_arrows: [OpenMP](https://www.openmp.org/) parallelized by default, with additional support for MPI parallelization
  - :package: Light dependency footprint 
  - :baby_bottle: Intuitive to use
  - :pager: command line interface

## Documentation

  - You can find the online documentation [here](https://sqsgenerator.readthedocs.io/en/latest/)
  - Learn how to [get started](https://sqsgenerator.readthedocs.io/en/latest/how_to.html)!
  - For a more in-depth insight, you can read our [research article](https://doi.org/10.1016/j.cpc.2023.108664)

### Appreciation
I would highly, appreciate, if you cite our [article](https://doi.org/10.1016/j.cpc.2023.108664) in case you use the 
software in your research. Here is the [BibTeX entry](citation.bib). You  Many thanks in advance :smile:

## Installation

### using conda
The easiest way to install *sqsgenerator* is to use conda package manager. *sqsgenerator* is deployed on the
*conda-forge* channel. To install use:

```bash
conda install -c conda-forge sqsgenerator
```

  - The version published on [Anaconda Cloud](https://anaconda.org/conda-forge/sqsgenerator) is capable of OpenMP parallelization only
  - [ase](https://wiki.fysik.dtu.dk/ase/), [pyiron](https://pyiron.org) and [pymatgen](https://pymatgen.org/) are not
    required to install *sqsgenerator*. However, we strongly encourage you to install at least one of them.
    *sqsgenerator* uses those packages as backends to export the generated structures

### building it yourself
On HPC systems where also MPI support, and optimized binaries are desirable, it's highly recommended to build
*sqsgenerator* yourself. An extensive [installation guide](https://sqsgenerator.readthedocs.io/en/latest/installation_guide.html)
can be found in the documentation. The following prerequisites are needed:

  - a C++17 enabled compiler, with OpenMP support
  - Python >= 3.6 <sup>*</sup>
  - numpy <sup>**</sup>
  - [boost](https://www.boost.org/) libraries,
    - [Boost.Python](https://www.boost.org/doc/libs/1_78_0/libs/python/doc/html/tutorial/index.html) compatible with
      your Python interpreter<sup>*</sup>
    - [Boost.Python (numpy)](https://www.boost.org/doc/libs/1_78_0/libs/python/doc/html/numpy/index.html) extensions
      compatible with you environment<sup>* ,**</sup>
