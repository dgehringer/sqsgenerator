
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



### Specifying you own compositions - $\text{Re}_{0.333}\text{W}_{0.667}$

(example-two)=

Suppose we want to move on different compositions, and want to distribute different numbers of tungsten and rhenium.
In this case we have to explicitly specify a {ref}`composition <input-param-composition>` parameter. Using this
directive we can exactly specify **which** and **how many** atoms should be distributed. We will slightly modify the
example from above.

```{warning} Package dependencies
In order to run this example you need to have either `ase` or `pymatgen` installed.
See {ref}`optional dependencies <optional-dependencies>` for more information
```

```{code-block} yaml
---
lineno-start: 1
caption: |
    Download the {download}`YAML file <_static/re-w.second.yaml>` and the {download}`B2 structure file <examples/b2.vasp>`
---
structure:
  file: b2.vasp
  supercell: [3, 3, 3]
iterations: 1e9
shell_weights:
  1: 1.0
composition:
  Re: 18
  W: 36
```

Again let's analyse the difference in the input file and what it is actually doing under the hood

  - **Line 2:** read the file **b2.vasp** from the disk. By default `ase` will be used to read the structure file.
    For more information see the {ref}`structure <input-param-structure>` parameter documentation
  - **Lines 7-9** distribute 18 Rhenium and 36 Tungsten atoms on the lattice positions.
    1. the B2 structure file contains 2 lattice position
    2. in **Line 3** we replicate it three times in all directions
    3. one needs to distribute $2 \times 3 \times 3 \times 3 = 54$ atoms on the lattice positions
    4. the number of distributed atoms **must match** the number lattice positions to occupy

The using `composition` parameter you can distribute any arbitrary sequence of atomic elements.
Suppose we want to create cells with an even more complicated composition e. g.
$\text{Re}_{12}\text{W}_{14}\text{Mo}_{14}\text{Ta}_{14}$ simple change `composition` section in the above example to:

```{code-block} yaml
---
lineno-start: 7
caption: |
    B2 structure with $\text{Re}_{12}\text{W}_{14}\text{Mo}_{14}\text{Ta}_{14}$ stochiometry
---
composition:
  Re: 12
  W: 14
  Mo: 14
  Ta: 14
```

### Perform SQS only on selected sites

#### Perform SQS on a sublattice only - $\text{Ti}\text{N} \rightarrow \text{Ti}_{0.5}(\text{B}_{0.25}\text{N}_{0.25})$

*sqsgenerator* allows you to select lattice positions, on which the SQS iteration is then carried out. This is done by
specifying a {ref}`which <input-param-which>` input parameter. All sites which are not explicitly chosen are ignored
during the optimization. The following example checks **all possible** configuration and will therefore an optimized
SQS structure

```{code-block} yaml
---
lineno-start: 1
emphasize-lines: 4, 7
caption: |
    Download the {download}`YAML file <examples/ti-n.yaml>` and the {download}`TiN structure file <examples/ti-n.cif>`
---
structure:
  supercell: [2, 2, 2]
  file: ti-n.cif
mode: systematic
shell_weights:
  1: 1.0
which: N
composition:
  B: 16
  N: 16
```

- **Line 4:** set the iteration mode to **systematic**. This will scan through all possible structures. **Note:** Check
  the size of the configuration space before actually running the minimization process. Otherwise, the program might
  run "*forever*"
- **Line 7:** use only nitrogen lattice positions to perform the SQS minimization.

This example generates all possible configurations ($\approx 6 \cdot 10^8$) and analyses them.
It is recommended to use `compute estimated-time` when using **systematic** iteration mode.

```{code-block} bash
> sqsgen compute total-permutations ti-n.yaml  # check the size of the configurational space
601080390
> sqsgen compute estimated-time ti-n.yaml  # estimate how long it will take
It will take me roughly 14 minutes and 23.576 seconds to compute 601080390 iterations (on 8 threads)
> sqsgen run iteration ti-n.yaml
```

#### $\gamma$-iron (austenite) - Partial random occupancy of interstitial atoms

The *sqsgenerator* also knows a fictitious atomic species "**0**", representing a vacancy. During the optimization
 vacancies will be treated as atoms. When exporting the structures the vacancies are deleted.

The following example constructs a $\gamma$-iron cell, where carbon is distributed on the **octahedral interstitial**
sites. Therefore, the {download}`structure file <examples/gamma-iron-octahedral.vasp>` contains four iron atoms and
four hydrogen (H) atoms on the octahedral sites.

```{code-block} yaml
---
lineno-start: 1
emphasize-lines: 7, 10
caption: |
    Download the {download}`YAML file <examples/gamma-iron.yaml>` and the {download}`iron structure file <examples/gamma-iron-octahedral.vasp>`
---
structure:
  supercell: [3, 3, 3]
  file: gamma-iron-octahedral.vasp
iterations: 1e8
shell_weights:
  1: 1.0
which: H
composition:
  C: 9
  0: 99
```

  - **Line 7:** hydrogen works here as a **dummy** species. We select those interstitial sites
  - **Line 10:** distribute nine carbon atoms and 99 vacancies

### Analyse existing structures
Sometimes it is desirable to compute the SRO parameters ($\alpha^i_{\xi\eta}$) for an exiting arrangement of atoms
rather than to generate a new one. To analyse existing structures *sqsgenerator* provides you with the "*analyse*" command.

#### Restore $\alpha^i_{\xi\eta}$ from structure files

**Note:** This example only works with `pymatgen` or `ase` installed

1. We use the {ref}`example above <example-two>` to generate some randomized structures by executing

   ```{code-block}  bash
   sqsgen run iteration --similar --no-minimal --export --dump-include objective --dump-include parameters re-w.second.yaml
   # or with shortcuts
   sqsgen run iteration -s -nm -e -di objective -di parameters re-w.second.yaml
   ```
2. The above command will store the optimized configurations in a file named *re-w.second.result.yaml*. The file will,
   in addition also contain (eventually) configurations which do not minimize (`--no-minimal/-nm`) the objective function
   Eq. {eq}`eqn:objective-actual`. Furthermore, it will not check for duplicates in the SRO formalism (`--similar/-s`).
   Finally *re-w.second.result.yaml* will contain the SRO parameters $\alpha^i_{\xi\eta}$
   (`--dump-include/-di parameters`) as well as the value of the objective function $\mathcal{O}$
   (`--dump-include/-di objective`). All the configurations will be also exported into *CIF* format (default). Listing
   your directory, should give you ten additional cif-files.
3. Please inspect *re-w.second.result.yaml* with a text editor
4. Now, the task is to reconstruct the SRO parameters from the exported cif-files. Therefore use:

   ```{code-block} bash
   sqsgen analyse *.cif
   ```

   The command will print the computed SRO parameters, nicely formatted to the console.
   **Note:** The output will show you SRO parameters for *seven* coordination shells with the default
   {ref}`shell_weights <input-param-shell-weights>` of $w^i = \frac{1}{i}$. This happens since *sqsgenerator* does not
   know the settings for computing the structures, hence it uses its default values.
5. To fix this, the `analyse` command takes a `--settings/-s` parameter. It points to a file providing the input settings.
   In this particular example we have two ways forward, to obtain the same values as in *re-w.second.result.yaml*:

      - create a new file *settings.yaml* with the following lines

        ```{code-block} yaml
        shell_weights:
          1: 1.0
        ```

        to take into account only the first coordination shell as {ref}`above <example-two>` and run

        ```{code-block} bash
        sqsgen analyse *.cif --settings settings.yaml
        ```

      - reuse the input file *re-w.second.yaml* and just execute

        ```{code-block} bash
        sqsgen analyse *.cif --settings re-w.second.yaml
        ```

        *sqsgenerator* will ignore all parameters which are not needed.


## Using Python API

## Templates
(templates)=
