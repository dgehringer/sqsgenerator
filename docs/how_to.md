
# How To

## Using the CLI interface

This section deals with the usage of the *sqsgenerator* package. A more granular documentation for the CLI can be found
in the {ref}`CLI Reference <cli_reference>`.

Once you have managed to {ref}`install <installation>` *sqsgenerator* you should have a command `sqsgen` available
in your shell.

Make sure you can call the `sqsgen` command before you start, using

```{code-block} bash
sqsgen --version
```

which should plot version information about *sqsgenerator* and also its dependencies.

### The `sqs.json` file

*sqsgenerator* uses a `dict`-like configuration, to store the parameters used for the optimization process. By
default, the program assumes the configuration to be stored in a JSON file named *sqs.json* in the current directory.

::::{tab} Python CLI
:::{code-block} bash
sqsgen run
:::
::::
::::{tab} Native CLI
:::{code-block} bash
sqsgen
:::
::::

If you want to use a different file name or path you can always pass it as an argument to the command, e. g.

::::{tab} Python  CLI
:::{code-block} bash
sqsgen run -i path/to/my/sqs.json
:::
::::

::::{tab} Native  CLI
:::{code-block} bash
sqsgen -i path/to/my/sqs.json
:::
::::


### Simple SQS - an ideal $\text{Re}_{0.5}\text{W}_{0.5}$ solution

In the following example we use a **Monte-Carlo** approach using by probing **fify million** different configurations.
Only **the first** coordination shell should be taken into account. We create super-cell with 54 atoms, by replicating
a simple B2 structure

:::{code-block} json
:class: runbutton
:lineno-start: 1

{
    "structure": {
        "lattice": [
            [3.165, 0.0, 0.0],
            [0.0, 3.165, 0.0],
            [0.0, 0.0, 3.165]
        ],
        "coords": [
            [0.0, 0.0, 0.0],
            [0.5, 0.5, 0.5]
        ],
        "species": ["W", "Re"],
        "supercell": [3, 3, 3]
    },
    "iterations": 50000000,
    "shell_weights": {
        "1": 1.0
    },
    "composition": {
        "W": 27,
        "Re": 27
    },
    "max_results_per_objective": 10
}
:::



So let's go together through this configuration:

  - **Lines 4-6:** cubic lattice with lattice parameter of $a_{bcc} = 3.165\;\text{A}$
  - **Lines 9-10:** lattice sites at positions $(0,0,0)$ and $(\frac{1}{2}, \frac{1}{2}, \frac{1}{2})$
  - **Line 12:** occupy the first site with Tungsten and the second one with Rhenium
  - **Line 13:** create a $3 \times 3 \times 3$ supercell
  - **Line 15:** choose $5^7$ different configurations randomly
  - **Lines 16-18:** use only the first coordination shell with a shell weight of $w_i=1.0$ {eq}`eqn:objective`. We have
    to explicitly state that. By default, SRO parameters in **all available** coordination shells {eq}`eqn:wc-sro-multi`
    are minimized at the same time
  - **Lines 19-22:** distribute 27 Tungsten and 27 Rhenium atoms on the lattice sites. The number of atoms to
    distribute must match the number of lattice sites to occupy.
  - **Line 23:** store at most 10 configurations per objective in the output file.


### Running an optimization


:::::{dropdown} 🚀 Templates
:color: primary
:open: true
Each of the examples below is packaged into *sqsgenerator* as a template.

Read more about them in the {ref}`templates <templates>` section.
To obtain the input file named *re-w.first* for this example file use:

::::{tab} Python CLI
:::{code-block} bash
sqsgen template use re-w.first # re-w.first is the template name
:::
::::

::::{tab} Native CLI
:::{code-block} bash
sqsgen template re-w.first # re-w.first is the template name
:::
::::
:::::


Run the current example using

::::{tab} Python  CLI
:::{code-block} bash
sqsgen run -i re-w.first.json
:::
::::

::::{tab} Native  CLI
:::{code-block} bash
sqsgen -i re-w.first.json
:::
::::

In case you have not passed a custom script the program will create an output file named `sqs.mpack`.
Otherwise, it will modify the passed filename e.g. `re-w.first.sqs.json` $\rightarrow$ `re-w.first.sqs.mapck`.


### The `sqs.mpack` file
(sqs-mpack)=

The output file `sqs.mpack` is a binary file in *MessagePack* format. It contains all the information about the
optimization process Those are:

  - input configuration
  - performance metrics
  - all computed structures (in a compressed format)
  - all computed SRO parameters

It holds all results grouped by the value of the objective function $\mathcal{O}(\sigma)$ {eq}`eqn:objective` in ascending order.

#### list the output

To view the results of an optimization use (in case you have an `sqsgen.mpack` file in your current directory):

::::{tab} Python CLI
:::{code-block} bash
sqsgen output list
:::
::::

::::{tab} Native CLI
:::{code-block} bash
sqsgen output
:::
::::

Similarly, in case our output file is named differently use e.g.

::::{tab} Python CLI
:::{code-block} bash
sqsgen output -o re-w.first.sqs.mpack list
:::
::::

::::{tab} Native CLI
:::{code-block} bash
sqsgen output -o re-w.first.sqs.mpack
:::
::::

You should see something like

:::{code-block} text

Mode: interact
min(O(σ)): 0.00000
Num. objectives: 5

INDEX  OBJ.     N
0      0.00000  13
1      0.01852  12
2      0.03704  2
3      0.07407  1
4      0.25926  1
:::



To export the **first** best structure use

::::{tab} Python CLI
:::{code-block} bash
sqsgen output structure -f cif
:::
::::

::::{tab} Native CLI
:::{code-block} bash
sqsgen output structure -f cif
:::
::::

You should see a file name `sqs-0-0.cif`. The naming convention is `sqs-{objective_index}-{structure_index}.{format}

In case you want to change the index and want to output the worst structure

::::{tab} Python CLI
:::{code-block} bash
sqsgen output structure -f cif --objective 4 --index 0
:::
::::

::::{tab} Native CLI
:::{code-block} bash
sqsgen output structure -f cif --objective 4 --index 0
:::
::::

##### file formats

The native CLI and can export:

  - CIF
  - POSCAR
  - PDB
  - JSON
    - ase
    - pymatgen
    - sqsgen

The python CLI will automatically detect *ase* and *pymatgen* in case one or both are installed.
To specify a file format use `-f {backend}.{format}`. E.g. you have *pymatgen* installed and want to use
it as write backend use `-f pymatgen.cif`. For a full list of available formats use `--help` switch.




### Perform SQS on a sublattice only - $\text{Ti}\text{N} \rightarrow \text{Ti}_{0.5}(\text{B}_{0.25}\text{N}_{0.25})$

*sqsgenerator* allows you to select lattice positions, on which the SQS iteration is then carried out. By
specifying the *{ref}`sublattice_mode <input-param-sublattice-mode>`* you can control how the optimization works.

:::::{tab} CLI/Python API
:::{code-block} json
:lineno-start: 1
:caption: Download the {download}`TiN structure file <_static/ti-n.vasp>`
{
  "iterations": 100000000,
  "sublattice_mode": "split",
  "structure": {
    "file": "ti-n.vasp",
    "supercell": [2, 2, 2]
  },
  "composition": [{
    "sites": "N",
    "B": 16,
    "N": 16
  }]
}
:::
:::::

:::::{tab} Web
:::{code-block} json
:class: runbutton

{
  "iterations": 100000000,
  "sublattice_mode": "split",
  "structure": {
    "lattice": [
      [4.253534, 0.0, 0.0],
      [0.0, 4.253534, 0.0],
      [0.0, 0.0, 4.253534]
    ],
    "coords": [
      [0.5, 0.0, 0.0],
      [0.5, 0.5, 0.5],
      [0.0, 0.0, 0.5],
      [0.0, 0.5, 0.0],
      [0.0, 0.0, 0.0],
      [0.0, 0.5, 0.5],
      [0.5, 0.0, 0.5],
      [0.5, 0.5, 0.0]
    ],
    "species": ["Ti", "Ti", "Ti", "Ti", "N", "N", "N", "N"],
    "supercell": [2, 2, 2]
  },
  "composition": [{
    "sites": "N",
    "B": 16,
    "N": 16
  }],
  "max_results_per_objective": 10
}
:::
:::::


- **Line 3:** sets the *{ref}`sublattice_mode <input-param-sublattice-mode>`* to *split*. The default is *interact*.
- **Line 5-6:** read the structure from a file named *ti-n.vasp* and create a $2 \times 2 \times 2$ supercell
- **Lines 8-12:** distribute 16 Boron and 16 Nitrogen atoms on the sites originally occupied by Nitrogen atoms only.
  The Titanium sublattice remains untouched. **Note:** the brackets around the composition.


#### the `sublattice_mode` parameter
For an in-depth discussion of the different modes please refer to the *{ref}`sublattice_mode <input-param-sublattice-mode>`* section.


  - *split*: the input structure is **splitted** into sublattices as defined in the *{ref}`composition <input-param-composition>`*
    The optimization is then carried out on the specified sublattices only. The use cases are:
    - restrict the optimization to a sublattice only (this example)
    - optimize multiple sublattices simultaneously in one run

  - *interact*: all atoms interact with each other. The SQS optimization is carried out on all atoms.
    You use this mode if you want to pin certain atoms to position.


In this example this results in N-N, B-N and B-B pairs being optimized whereas. If we had chosen *interact* mode Ti-Ti, Ti-N and Ti-B would be subject to the optimization as well.


#### specifying structure files
you can read in structure files using the *file* key in the *structure* section.

  - the web version does not support file uploads
  - the native CLI supports only POSCAR and pymatgen JSON and *sqsgenerator* JSON files
    - the extension determines the file format e.g. `ti-n.pymatgen.json` or `ti-n.sqs.json`
  - the python CLI supports all file formats of the native CLI plus what ever *ase* or *pymatgen* supports
    - the extension determines the file format and is `{backend}.{format}`
    - e.g. `ti-n.ase.cif` (use *ase* as backend to read a CIF file) or `ti-n.pymatgen.cif` (use *pymatgen* as backend to read a CIF file)
    - no backend means `sqs`
    - to get a list of all available extensions use `sqsgen export structure --help`

## Using Python API

In case you use the Python CLI, you have also might want to use the Python API. The API itself is minimalistic
It contains three main functions which can imported from the `sqsgenerator` package

:::{code-block} python
from sqsgenerator import parse_config, optimize, load_result_pack
:::

`parse_config` accepts both a JSON string or a `dict` configuration and returns a config object.

To run an optimization use

:::{code-block} python
from sqsgenerator import parse_config, optimize

with open("re-w.first.json") as f:
    config = parse_config(f.read())

pack = optimize(config)
:::

When specifying a lattice or the coords also numpy arrays are accepted

### loading existing results

In case you have an existing `sqs.mpack` from the webapp or the native CLI file you can load it using

:::{code-block} python
from sqsgenerator import load_result_pack

with open("sqs.mpack", "rb") as f:
    pack = load_result_pack(f.read())
:::

### analysing the results

`optimize` returns the structures in a packed format. You can think of it as  `list[tuple[float, list[SqsResult]]]` where
each entry contains the solutions with the same objective {eq}`eqn:objective-actual` value in *ascending* order.

To obtain the best structure use

:::{code-block} python
best = pack.best()
:::

while is basically equivalent to `pack[0][0]`. To see a list of all objectives and the number of structures for each use

:::{code-block} python
for obj, solutions in pack:
    print(f"Objective: {obj}, Num. solutions: {len(solutions)}")
:::

### exporting structures

Each soution is of type `SqsResult` which contains the structure and the computed SRO parameters. You can access the structure
using the `structure` method. To export all structures you could use (here we choose CIF with *pymatgen* as backend):

:::{code-block} python
from sqsgenerator import write

for oi, (obj, solutions) in enumerate(pack):
    for si, solution in enumerate(solutions):
        write(solution.structure(), f"sqs-{oi}-{si}.pymatgen.cif")
:::


## Templates
(templates)=
