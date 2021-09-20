
# How to use `sqsgenerator`

This is short tutorial, on how `sqsgenerator` works


## using the CLI interface

This section deals with the usage of the `sqsgenerator` package. A more granular documentation for the CLI can be found 
in the {ref}`CLI Reference <cli_reference>`. The CLI interface was built using the excellent [click](https://click.palletsprojects.com/en/8.0.x/) framework.

Once you have managed to {ref}`install <installation_guide>` `sqsgenerator` you should have a command `sqsgen` available
in your shell.

Make sure you can call the `sqsgen` command before you start, using

```{code-block} bash
sqsgen --version
```

which should plot version information about `sqsgenerator` and also its dependencies.

### the `sqs.yaml` file

`sqsgenerator` uses a `dict`-like configuration, to store the parameters used for the iteration/analysis process. By
default the program assumes the configuration to be stored in a YAML file.

YAML is easy to read and write by humans. On this tutorial site we will focus on setting up proper `sqs.yaml` input 
files.

Most of the CLI commands which require an input settings file. However, specifying an file is always optional, since 
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
sqsgen run iteration path/to/my/custom/sqs_config.yaml
```

This applies to nearly all commands, except `sqsgen analyse` which by default expects **sqs.results.yaml**

````

### Simple SQS

#### Refactory metals - an ideal $\text{Re}_{0.5}\text{W}_{0.5}$ solution

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

In the unit cell we do have a 50-50 composition. Replication does not change to chemistry, thus we end up with with 27 
tungsten atoms and 27 rhenium atoms in the final configuration.

````{note} 

You can always check, which atomic species are distributed on the lattice sites using the `params show` command

```{code-block} bash
sqsgen params check --parameter composition
```

````

#### moving to different compositions $\text{Re}_{0.333}\text{W}_{0.667}$

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
Suppose we want to create cells with a even more complicated composition e. g. 
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

### A note on the number of `iterations`

Actually it is very hard to tell what is a "**sufficiently**" large enough number for the `iteration` parameter. As the 
configurational space is growing extremly fast (factorial), it is anyway not possible to sample it properly in case the
sructures get large enough.

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

    You can tune the number of permutations to a computing time you can afford. The above command gives only a estimate
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

