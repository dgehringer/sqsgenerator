
# Welcome to *sqsgenerator*'s documentation!


## Highlights
- 🚀 Blazingly fast short-range-order calculations (C++ core)
- ➰ Monte-Carlo and systematic approach to compute optimal atomic configuration
- 🧵multithreaded by default (optional MPI support) also in the browser 🌐
- 🔀 optimize multiple sublattices simultaneously in a single run
- 🔌 easy integration with other frameworks ([*ase*](https://wiki.fysik.dtu.dk/ase/),
    [*pymatgen*](https://pymatgen.org/) and [*pyiron*](https://pyiron.org/))


## Get started

 - 🌐 [directly in your browser](https://sqsgen.gehringer.tech) (multithreaded WebAssembly, no installation required)
 - 🐍 install the Python package
   - via {ref}`pip <pip-installation>`: `pip install sqsgenerator`
   - via {ref}`conda <conda-installation>`: `conda install -c conda-forge sqsgenerator`
 - 💻 build an MPI enabled {ref}`native app <native-installation>` for HPC environments

::::{dropdown} 🎓 Cite us
:color: primary
:open: true

If you use *sqsgenerator* in your research, please cite our [research article](https://doi.org/10.1016/j.cpc.2023.108664):

```bibtex
@article{sqsgen,
  doi = {10.1016/j.cpc.2023.108664},
  url = {https://doi.org/10.1016/j.cpc.2023.108664},
  year = {2023},
  month = jan,
  publisher = {Elsevier {BV}},
  pages = {108664},
  author = {Dominik Gehringer and Martin Fri{\'{a}}k and David Holec},
  title = {Models of configurationally-complex alloys made simple},
  journal = {Computer Physics Communications}
}
```
::::


## Who is using *sqsgenerator*?

::::{grid} 3
:gutter: 2

:::{grid-item-card} RWTH Aachen
:img-top: https://www.mch.rwth-aachen.de/global/show_picture.asp?id=aaaaaaaaaaotqbp
:link: https://www.mch.rwth-aachen.de
:class-img-top: affiliation

Materials Chemistry

:::

:::{grid-item-card} Linköping University
:img-top: https://upload.wikimedia.org/wikipedia/commons/thumb/a/a5/Linkoping_university_logo15.png/400px-Linkoping_university_logo15.png
:link: https://liu.se/en/organisation/liu/ifm/tunnf
:class-img-top: affiliation

Thin Film Physics
:::

:::{grid-item-card} TU Vienna
:img-top: images/tu_wien.svg
:link: https://wwwt.tuwien.ac.at/
:class-img-top: affiliation

Institute of Materials Science and Technology
:::


:::{grid-item-card} TU Vienna
:img-top: images/tu_wien.svg
:link: https://www.iue.tuwien.ac.at/home
:class-img-top: affiliation

Institute of Microelectronics
:::

:::{grid-item-card} Czech Academy of Sciences
:img-top: https://www.ipm.cz/wp-content/uploads/2018/01/logo.png
:link: https://www.ipm.cz/
:class-img-top: affiliation

Institute of Physics of Materials
:::

:::{grid-item-card} Technical University Brno
:img-top: https://upload.wikimedia.org/wikipedia/commons/thumb/a/a5/VUT_CZ.svg/512px-VUT_CZ.svg.png
:link: https://www.fme.vutbr.cz/
:class-img-top: affiliation

Institute of Materials Science and Engineering
:::

:::{grid-item-card} Montanuniversität Leoben
:img-top: https://www.unileoben.ac.at/fileadmin/shares/allgemeine_datenbank/logos/1-mul-logo/2-mul-logo-smoke-quer.svg
:link: https://cms.unileoben.ac.at/
:class-img-top: affiliation

Computational Materials Science
:::

:::{grid-item-card} Max Planck Institute for Sustainable Materials
:img-top: https://www.mpg.de/assets/og-logo-281c44f14f2114ed3fe50e666618ff96341055a2f8ce31aa0fd70471a30ca9ed.jpg
:link: https://mpie.de/
:class-img-top: affiliation

CM Department
:::


:::{grid-item-card} ams OSRAM AG
:img-top: https://upload.wikimedia.org/wikipedia/commons/thumb/e/e7/Ams-Osram_Logo.svg/250px-Ams-Osram_Logo.svg.png
:link: https://ams-osram.com
:class-img-top: affiliation

::::




## Table of contents

:::{toctree}
---
maxdepth: 1
---

installation
how_to
parameters
under_the_hood
cli_reference

:::

## API Reference
* {ref}`genindex`
* {ref}`search`
