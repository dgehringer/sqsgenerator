
# How to use `sqsgenerator`

This is short tutorial, on how `sqsgenerator` works


## Using the CLI interface

This section deals with the usage of the `sqsgenerator` package. A more granular documentation for the CLI can be found 
in the {ref}`CLI Reference <cli_reference>`. The CLI interface was built using the excellent [click](https://click.palletsprojects.com/en/8.0.x/) framework.

Once you have managed to {ref}`install <installation_guide>` `sqsgenerator` you should have a command `sqsgen` available
in your shell.

Make sure you can call the `sqsgen` command before you start, using

```{code-block} bash
sqsgen --version
```

which should plot version information about `sqsgenerator` and also its dependencies.

### The `sqs.yaml` file

`sqsgenerator` uses a `dict`-like configuration, to store the parameters used for the iteration/analysis process. By
default the program assumes the configuration to be stored in a YAML file.

YAML is easy to read and write by humans. On this tutorial site we will focus on setting up proper `sqs.yaml` input 
files.

Most of the CLI commands which require an input settings file. However, specifying a file is always optional, since 
`sqsgenerator` will always look for a default file name `sqs.yaml` in the **current directory**.

`sqsgenerator` an also read more formats, which can store Python `dict`s, such as **JSON** and **pickle**. Therefore, 
all commands which require a settings file also do have an `--input-fomat` (`-if`) option, which instruct the program 
to use different file-formats. For more infos please have look at the {ref}`CLI Reference <cli_reference>`.


````{note}

Most of `sqsgen`'s subcommands need such an input file to obtain the parameters.

By default the program searches for a file named `sqs.yaml` in the directory where it is executed
You can always specify a different one in case you want the program to execute a different one.

```{code-block} bash
sqsgen run iteration  # is equal to
sqsgen run iteration sqs.yaml
sqsgen run iteration path/to/my/custom/sqs_config.yaml  # or pass a custom file
```

This applies to nearly all commands, except `sqsgen analyse` which by default expects 
{ref}`sqs.result.yaml <sqs-result-yaml>`

````

### Simple SQS

#### Simple SQS - an ideal $\text{Re}_{0.5}\text{W}_{0.5}$ solution

In the following example we use a **Monte-Carlo** approach using by probing **one billion** different configurations.
Only **the first** coordination shell should be taken into account. We create super-cell with 54 atoms, by replicating
a simple B2 structure

```{code-block} yaml
---
lineno-start: 1
caption: |
    Download the input {download}`YAML file <examples/re-w.first.yaml>`
---
structure:
  lattice:
    - [3.165, 0.0, 0.0]
    - [0.0, 3.165, 0.0]
    - [0.0, 0.0, 3.165]
  coords:
    - [0.0, 0.0, 0.0]
    - [0.5, 0.5, 0.5]
  species: [W, Re]
  supercell: [3, 3, 3]
iterations: 1e9
shell_weights:
  1: 1.0
```

So let's go together through this configuration:

  - **Line 2:** create a cubic lattice with a lattice parameters of $a_{bcc} = 3.165\;\text{A}$
  - **Lines 7-8:** Place two lattice sites at positions $(0,0,0)$ and $(\tfrac{1}{2}, \tfrac{1}{2}, \tfrac{1}{2})$
  - **Line 9:** Occupy the first site with Tungsten and the second one with Rhenium
  - **Line 10:** Replicate this unit cell three times into $a$, $b$ and $c$ direction
  - **Line 11:** Test $10^9$ different configurations
  - **Lines 12-13:** Use only the first coordination shell with a shell weight of $w_i=1.0$ {eq}`eqn:objective`. We have
    to explicitly state that. By default, SRO parameters in **all available** coordination shells {eq}`eqn:wc-sro-multi`
    are minimized at the same time

In the unit cell we do have a 50-50 composition. Replication does not change to chemistry, thus we end up with 27 
tungsten atoms and 27 rhenium atoms in the final configuration.

````{note} 

You can always check, which atomic species are distributed on the lattice sites using the `params show` command

```{code-block} bash
sqsgen params show --parameter composition
```

````

#### Running an optimization

Once you have created a YAML input file you can run an optimization
In case you have downloaded the above example you can run it using

```{code-block} bash
sqsgen run iteration re-w.first.yaml
```

In case you have not passed a custom script the programm will create an output file named `sqs.result.yaml`.
Otherwise it will modify the passed filename e. g. `re-w.first.yaml` $\rightarrow$ `re-w.first.result.yaml`


#### The `sqs.result.yaml` file
(sqs-result-yaml)=

The `*.result.yaml` files are used to dump the output of the optimization process. The file contains the following 
entries:

 - `structure`: the structure read from the input file in expanded format
 - `which`: the list of selected lattice positions
 - `timings`: runtime information, saved in `dict` like format. The keys are integer numbers and identify the MPI rank.
              In case you **do not have** MPI enabled version it contains always only one entry with key `0`. The 
              numbers represent the average time a thread needs to analyse a structure and generate the next one.
              The times are in **µs**, while the index in the value list corresponds to the thread ID.
 - `configurations`: the computed SQS results in a dict-like manner. The keys are the [rank](https://stackoverflow.com/questions/22642151/finding-the-ranking-of-a-word-permutations-with-duplicate-letters)
              of the permutation sequence. The values are a sequence of atomic symbols.


##### How many structures are actually computed?
The number of structures in `sqs.result.yaml` is basically determined by the 
{ref}`max_output_configurations <input-param-max-output-configurations>` parameters which is by default 10.
There is however as post-processing step **after the minimization** process. The default behaviour `sqsgenerator` is 
to discard those configurations which do not exhibit the minimal values of the objective function. Furthermore, our 
definition of the objective function in Eq. {eq}`eqn:objective` may yield "*degenerate*" results, which are also 
discarded in the post-processing step. This "*degeneracy*" decreases by including more coordination shells.

 - To include degenerate structures you can use the ` --no-similar`/`-ns` switch
 - To include structures eventually with non-optimal objective function use  `--no-minimal`/`-nm` switch


````{admonition} Include objective function $\mathcal{O}(\sigma)$ and SRO parameters $\alpha^i_{\xi\eta}$
:class: tip

To dump the also the objective function $\mathcal{O}(\sigma)$ {eq}`eqn:objective` and SRO parameters 
$\alpha^i_{\xi\eta}$ {eq}`eqn:wc-sro-multi` you can use the `--dump-include`/`-di` switches. You need to explicitly 
specify each quantity.

```{code-block} bash
sqsgen run iteration --dump-include parameters --dump-include objective
```

will include both quantities in `sqs.result.yaml`. This will however modify the structure of the
`configuration` key in `sqs.result.yaml`.

````

#### Export the computed structures

To obtain the structures stored in `sqs.result.yaml` the `export` command should be used. This command searches for a 
**sqs.result.yaml** if not specified.

```{code-block} bash
sqsgen export re-w.first.result.yaml
```

will export all the structures in **cif** format. 

  - The filename will be the rank of the permutation. 
  - You can specify a different output format using `--format`/`-f` switch.
  - You can explicitly specify the backend with the `--writer`/`-w` switch. If not specified otherwise the **ase** 
    backend will be used 
  - To gather the structure files in an archive use the `--compress`/`-c` switch

````{admonition} Directly export the structure
:class: tip

The `export` command is optional. Structures can be exported directly using the `run iteration` command if 
a `--export`/`-e` switch is passed.

If the `--export` switch is passed, the above mentioned switches 
(`--format`/`-f`, `--writer`/`-w` and `--compress`/`-c`) can be using in combination with the `run iteration` command

```{code-block} bash
sqsgen run iteration -e -f vasp -c xz -w pymatgen
```
will run an iteration and export all computed structures in **POSCAR** format using **pymatgen** in an archive 
**sqs.results.tar.xz**

````

#### Specifying you own compositions - $\text{Re}_{0.333}\text{W}_{0.667}$

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
    Download the {download}`YAML file <examples/re-w.second.yaml>` and the {download}`B2 structure file <examples/b2.vasp>`
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

`sqsgenerator` allows you to select lattice positions, on which the SQS iteration is then carried out. This is done by 
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
  the size of the configurational space before actually running the minimization process. Otherwise, the program might 
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

The `sqsgenerator` also knows a fictitious atomic species "**0**", representing a vacancy. During the optimization
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

### Advanced topics

```{admonition} This section will come in future
:class: note

This section needs still to be done
 - Custom `target_objective`
 - Maybe MPI enabled version
```

### A note on the number of `iterations`

Actually it is very hard to tell what is a "**sufficiently**" large enough number for the `iteration` parameter. As the 
configurational space is growing extremely fast (factorial), it is anyway not possible to sample it properly in case the
structures get large enough.

To get a feeling how many structures are there, set `mode` to **systematic** and hit

```{code-block} bash
sqsgen compute total-permutations
```

This will print you the number of different structures one can construct. This number might be really huge, 
however lots of the might be symmetrically equivalent.

A few rules over the thumb, and what you can do if you deal with "*large*" systems

  - Check how long it would take to compute your current settings
  
    ```{code-block} bash
    sqsgen compute estimated-time
    ```

    You can tune the number of permutations to a computing time you can afford. The above command gives only an estimate
    for the current machine.
    
  - Reduce the number of shells. This has two-fold advantage
    1. In contrast to old versions of `sqsgenerator`, the current implementations profit greatly from a decreased number
       of coordination shells. The actual speedup depends on the input structure but might be up to an order of 
       magnitude when compared to the default value (all shells are considered)
       ```{image} images/time_vs_shells.svg
       :alt: Estimated time vs. number of coordination shells
       :width: 67%
       :align: center
       ```
    3. The image size of the objective function is drastically reduced. In other words a lot of different structures are 
       mapped onto the same value of the objective function.

