![build](https://git.unileoben.ac.at/p1655622/sqsgenerator/badges/master/pipeline.svg?style=flat&key_text=build)
![tests](https://git.unileoben.ac.at/p1655622/sqsgenerator/badges/master/pipeline.svg?style=flat&key_text=tests)
![coverage](https://git.unileoben.ac.at/p1655622/sqsgenerator/badges/master/coverage.svg) [![Documentation Status](https://readthedocs.org/projects/sqsgenerator/badge/?version=latest)](https://sqsgenerator.readthedocs.io/en/latest/?badge=latest)


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
  - Learn how to [get started](https://sqsgenerator.readthedocs.io/en/latest/how_to.html)

## Requirements

To install sqsgenerator you need the following things:

  - a Python >= 3.6 distribution
  - [numpy](https://numpy.org) installed
 
[ase](https://wiki.fysik.dtu.dk/ase/) and [pymatgen](https://pymatgen.org/) are not required, however we strongly encourage you to install at least one of them. sqsgenerator uses those packages as backends to export the generated structures

## Installation

An extensive [installation guide](https://sqsgenerator.readthedocs.io/en/latest/installation_guide.html) can be found in the documentation.

For Linux users we provide pre-built [wheels](https://sqsgenerator.readthedocs.io/en/latest/installation_guide.html#with-pip). Just download the proper whl-file and install it using e. g

```bash
pip install sqsgenerator-0.1-cp39-cp39-linux_x86_64.whl
```

In near future we will also update the conda-forge build
