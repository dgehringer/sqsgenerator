
# Welcome to `sqsgenerator`'s documentation!

```{admonition} Version mismatch with **conda-forge** channel
:class: danger

The `sqsgenerator` package was rewritten (nearly from scratch) and ported from C to C++. Therefore, it is still
under evaluation, while the **conda-forge** channel does contain still the old version of the package.

**This documentation** page **does not apply** to the old version, but rather to actual version of the package as it 
is found [here](https://github.com/dgehringer/sqsgenerator)

```

## Highlights

  - Easy integration with popular frameworks such as [ase](https://wiki.fysik.dtu.dk/ase/),
    [pymatgen](https://pymatgen.org/) and [pyiron](https://pyiron.org/)
  - Monte-Carlo and systematic approach to compute optimal atomic configuration
  - **Carefully hand-crafted** low-level C++ routines, for efficient calculation of short-range-order
  - [OpenMP](https://www.openmp.org/) parallelized by default, with additional support for MPI parallelization
  - Light dependency footprint 
  - Intuitive to use
  - Command line interface
  - Simple to use Python API

## Table of contents

```{toctree}
---
caption: 
maxdepth: 1
---

installation_guide
how_to
cli_interface
input_parameters
what_is_actually_computed
api_reference
```

## API Reference

* {ref}`genindex`
* {ref}`modindex`
* {ref}`search`
