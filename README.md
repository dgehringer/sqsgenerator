![plats](https://anaconda.org/conda-forge/sqsgenerator/badges/platforms.svg)
![downloads](
https://anaconda.org/conda-forge/sqsgenerator/badges/downloads.svg)
[![Documentation Status](https://readthedocs.org/projects/sqsgenerator/badge/?version=latest)](https://sqsgenerator.readthedocs.io/en/latest/?badge=latest)


<p align="center">
  <img src="https://github.com/dgehringer/sqsgenerator/raw/main/docs/images/logo_large.svg" width="60%" alt="sqsgenerator-logo" />
  <br /><br />
  <a href="https://sqsgen.gehringer.tech">  <img src="https://icon.icepanel.io/Technology/svg/WebAssembly.svg" alt="WebAssembly icon" height="16" width="16" /> Web Application</a> <br />
  <a href="https://sqsgenerator.readthedocs.io">📝 docs</a>
</p>

---

**sqsgenerator** is a Python package, which allows you to efficiently generate optimised *Special-Quasirandom-Structures* (SQS). The package uses *Warren-Cowley Short-Range-Order* (SRO) parameters to quantify randomness. The core routines are written in C++

## Highlights

- 🚀 Blazingly fast short-range-order calculations (C++ core)
- ➰ Monte-Carlo and systematic approach to compute optimal atomic configuration
- 🧵multithreaded by default (optional MPI support) also in the browser 🌐
- 🔀 optimize multiple sublattices simultaneously in a single run
- 🔌 easy integration with other frameworks ([*ase*](https://wiki.fysik.dtu.dk/ase/),
  [*pymatgen*](https://pymatgen.org/) and [*pyiron*](https://pyiron.org/))

## Installation

### start directly

Start directly in your browser  without any installation at 🌐 [**sqsgen.gehringer.tech**](https://sqsgen.gehringer.tech).

You can preview the results, download single files or the entire optimization for further analysis on your local machine.
The WebAssembly powered application is multithreaded and runs completely in your at near native speed.

### via `pip`

You can install the latest release of *sqsgenerator* from [PyPI](https://pypi.org/project/sqsgenerator/) using `pip`:

```bash
pip install sqsgenerator
```


### using conda
The easiest way to install *sqsgenerator* is to use conda package manager. *sqsgenerator* is deployed on the
*conda-forge* channel. To install use:

```bash
conda install -c conda-forge sqsgenerator
```


### Native application (MPI)

Since version 0.4 a native application is available, which can be used in HPC environments. The application is MPI enabled and can be built from source. Please refer to the [installation instructions](https://sqsgenerator.readthedocs.io/installation.html#native-application) for more details.

### Cite us
In case you use the software in your research, please cite our [article](https://doi.org/10.1016/j.cpc.2023.108664). Here is the [BibTeX entry](citation.bib).

## Documentation

  - You can find the online documentation [here](https://sqsgenerator.readthedocs.io/en/latest/)
  - Learn how to [get started](https://sqsgenerator.readthedocs.io/en/latest/how_to.html)!
  - For a more in-depth insight, you can read our [research article](https://doi.org/10.1016/j.cpc.2023.108664)
