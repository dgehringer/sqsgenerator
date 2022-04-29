
# How to use *sqsgen*?

This is short tutorial, on how *sqsgenerator* works


## Using the CLI interface

This section deals with the usage of the *sqsgenerator* package. A more granular documentation for the CLI can be found 
in the {ref}`CLI Reference <cli_reference>`. The CLI interface was built using the excellent [click](https://click.palletsprojects.com/en/8.0.x/) framework.

Once you have managed to {ref}`install <installation_guide>` *sqsgenerator* you should have a command `sqsgen` available
in your shell.

Make sure you can call the `sqsgen` command before you start, using

```{code-block} bash
sqsgen --version
```

which should plot version information about *sqsgenerator* and also its dependencies.

### The `sqs.yaml` file

*sqsgenerator* uses a `dict`-like configuration, to store the parameters used for the iteration/analysis process. By
default the program assumes the configuration to be stored in a YAML file.

YAML is easy to read and write by humans. On this tutorial site we will focus on setting up proper `sqs.yaml` input 
files.

Most of the CLI commands which require an input settings file. However, specifying a file is always optional, since 
*sqsgenerator* will always look for a default file name `sqs.yaml` in the **current directory**.

*sqsgenerator* an also read more formats, which can store Python `dict`s, such as **JSON** and **pickle**. Therefore, 
all commands which require a settings file also do have an `--input-fomat` (`-if`) option, which instruct the program 
to use different file-formats. For more infos please have look at the {ref}`CLI Reference <cli_reference>`.


````{admonition} *sqsgenerator* searches for a *sqs.yaml* file
:class: note, dropdown

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

````{admonition} Check which species are actually distributed
:class: tip, dropdown

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
There is however as post-processing step **after the minimization** process. The default behaviour *sqsgenerator* is 
to discard those configurations which do not exhibit the minimal values of the objective function. Furthermore, our 
definition of the objective function in Eq. {eq}`eqn:objective` may yield "*degenerate*" results, which are also 
discarded in the post-processing step. This "*degeneracy*" decreases by including more coordination shells.

 - To include degenerate structures you can use the ` --no-similar`/`-ns` switch
 - To include structures eventually with non-optimal objective function use  `--no-minimal`/`-nm` switch


````{admonition} Include objective function $\mathcal{O}(\sigma)$ and SRO parameters $\alpha^i_{\xi\eta}$
:class: tip, dropdown

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
:class: tip, dropdown

The `export` command is optional. Structures can be exported directly using the `run iteration` command if 
a `--export`/`-e` switch is passed.

If the `--export` switch is passed, the above mentioned switches 
(`--format`/`-f`, `--writer`/`-w` and `--compress`/`-c`) can be using in combination with the `run iteration` command

```{code-block} bash
sqsgen run iteration -e -f poscar -c xz -w pymatgen
```
will run an iteration and export all computed structures in **POSCAR** format using **pymatgen** in an archive 
**sqs.results.tar.xz**

````

#### Specifying you own compositions - $\text{Re}_{0.333}\text{W}_{0.667}$

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

### Advanced topics

```{admonition} This section will come in future
:class: note, dropdown

This section needs still to be done
 - Custom `target_objective`
 - Maybe MPI enabled version
```

#### multiple independent sublattices - From $\text{TiN} \rightarrow \left(\text{Ti}_{0.25} \text{Al}_{0.25} \right) \left( \text{B}_{0.25} \text{N}_{0.25} \right)$

Now we want to enhance the above examples, and want to distribute both Ti and Al on the original Ti sublattice. 
Furthermore, additionally, we want to distribute B and N on the original nitrogen sublattice. Before going into detail,
please download the example input files, the {download}`B2 structure <examples/ti-n.cif>` 
as well as the {download}`YAML input <examples/ti-al-b-n.yaml>`.

Before we start let's investigate the coordination shells of the input structure

```{code-block} bash
sqsgen compute shell-distances ti-al-b-n.yaml
[0.0, 2.126767, 3.0077027354075403, 3.6836684998608384, 4.253534, 4.755595584303295, 5.209493951789751, 6.015405470815081, 6.380301, 7.367336999721677]
```

we can interpret this number is the following way

```{image} images/ti-n.svg
:alt: Cooridnation shells in the TiN structure
:width: 33%
:align: center
```

Within the sphere of the first coordination shell ($R_1 \approx 2.12\; \mathring A$) of a Ti atom there are only N atoms.
While versa holds true too, within the second coordination shell ($R_2 \approx 3.08\; \mathring A$) there are only 
atoms of the same type.

```{code-block} yaml
---
lineno-start: 1
emphasize-lines: 6,8-15
caption: |
    Download the {download}`YAML file <examples/ti-al-b-n.yaml>` and the {download}`B2 structure <examples/ti-n.cif>` 
---
structure:
  supercell: [2, 2, 2]
  file: ti-n.cif
iterations: 5e6
shell_weights:
  2: 1.0
composition:
  B:
    N: 16
  N:
    N: 16
  Ti:
    Ti: 16
  Al:
    Ti: 16
```

The specification of the `composition` tag now differs from the previous examples

  - **Line 6:** Only consider second shell. Therefore, we automatically eliminate the Ti-B, Ti-N, Al-Ti and Al-V 
    interactions
  - **Line 8-11:** Deals with the (original) N sublattice, you can interpret the input int the following way
    - **Line 8-9:** Distribute **B** atoms on the original **N** sublattice and put **16** of there 
    - **Line 10-11:** Distribute **N** atoms on the original **N** sublattice and put **16** of there 
  - **Line 12-15:** Deals with the (original) Ti sublattice, you can interpret the input int the following way
    - **Line 12-13:** Distribute **Ti** atoms on the original **Ti** sublattice and put **16** of there 
    - **Line 10-11:** Distribute **Al** atoms on the original **Ti** sublattice and put **16** of there 

Since in the example above we care only about the second coordination shell, we eliminate all interactions between the 
two sublattices. Let's examine the output, therefore run the above example with

```{code-block} bash
sqsgen run iteration --dump-include objective --dump-include parameters ti-al-b-n.yaml
```

Using those options will also store the Short-Range-Order parameters $\alpha^i_{\xi\eta}$ and the value of the objective
function $\mathcal{O}(\sigma)$ in the resulting yaml file.

Those parameters should look something like this

```{code-block} yaml
objective: 5.4583
parameters:
- - [-0.0625, -0.875, 1.0, 1.0]
  - [-0.875, -0.0625, 1.0, 1.0]
  - [1.0, 1.0, -0.2083, -0.583]
  - [1.0, 1.0, -0.583, -0.2083]
```

in the `ti-al-b-n.result.yaml` file.

The atomic species are in ascending order with respect to their ordinal number. For this example it is B, N, Al, Ti
The above SRO parameters are arranged in the following way 
(see {ref}`target_objective <input-param-target-objective>` parameter).

```{math}
\boldsymbol{\alpha} = \left[
\begin{array}{cccc}
\alpha_{\text{B-B}} & \alpha_{\text{B-N}} & \alpha_{\text{B-Al}} & \alpha_{\text{B-Ti}} \\
\alpha_{\text{N-B}} & \alpha_{\text{N-N}} & \alpha_{\text{N-Al}} & \alpha_{\text{N-Ti}} \\
\alpha_{\text{Al-B}} & \alpha_{\text{Al-N}} & \alpha_{\text{Al-Al}} & \alpha_{\text{Al-Ti}} \\
\alpha_{\text{Ti-B}} & \alpha_{\text{Ti-N}} & \alpha_{\text{Ti-Al}} & \alpha_{\text{Ti-Ti}} \\
\end{array}
\right]
```

We immediately see that the SRO is 1.0 if the constituting elements sit on different sublattices. This is because
there are no pairs there $N_{\xi\eta}^2 = 0$. 

```{admonition} "*Wrong*" SRO parameters
:class: warning

Although the above example works and computes results, it does it not in a way we would expect it. Your can clearly 
observe this by $\alpha_{\text{N-B}} < 0$ and $\alpha_{\text{Al-Ti}} < 0$

The SRO parameters are not wrong but mean something differently. We have restricted each of the species to only half 
of the lattice positions. In other words from the 64 initial positions Ti and Al are only free to choose 32 two of them
(the former Ti sublattice). 

Let's consider Ti and Al for this particular example. According to Eq. {eq}`eqn:wc-sro` the SRO $\alpha_{\text{Al-Ti}}$
is defined as

$$
\alpha_{\text{Al-Ti}} = 1 - \dfrac{N_{\text{Al-Ti}}}{NMx_{\text{Al}}x_{\text{Ti}}} = 1 - \dfrac{\text{actual number of pairs}}{\text{expected number of pairs}}
$$

In the example above $N=64$ and $M^2=12$ while $x_{\text{Al}} = x_{\text{Ti}}=\frac{1}{4}$ which leads to 48 **expected** 
Al-Ti pairs

However, Al and Ti are not allowed to occupy all $N$ sites but only rather $\frac{N}{2}$ (Same is also true for B and N)
In addition the $\frac{N}{2}$ sites, are allowed to be occupied only by Al and Ti, consequently we have to change
$x_{\text{Al}} = x_{\text{Ti}}=\frac{1}{2}$. This however leads to **96 expected bonds**.

```

To fix this problem the above example need to be modified in the following way

```{code-block} yaml
---
lineno-start: 1
emphasize-lines: 16,17
caption: |
    Download the fixed {download}`YAML file <examples/ti-al-b-n-prefactors.yaml>` and the {download}`B2 structure <examples/ti-n.cif>` 
---
structure:
  supercell: [2, 2, 2]
  file: ti-n.cif
iterations: 5e6
shell_weights:
  2: 1.0
composition:
  B:
    N: 16
  N:
    N: 16
  Ti:
    Ti: 16
  Al:
    Ti: 16
prefactors: 0.5
prefactor_mode: mul
```

- **Line 16:** As the number of expected bonds changes, in the case of independent sublattices from 48 to 96. The
  "*prefactors*" $f_{\xi\eta}^i$ (see Eq. {eq}`eqn:prefactors`) represent the **reciprocal** value of the number of 
  expected bonds and therefore need to be multiplied by $\frac{1}{2}$ as $\frac{1}{2}\cdot \frac{1}{48} \rightarrow \frac{1}{96}$
- **Line 17:** "*mul*" means that the **default values** of $f_{\xi\eta}^i$ are **mul**tiplied with the values supplied
  from the {ref}`prefactor <input-param-prefactors>` parameters. $\tilde{f}^i_{\xi\eta} = \frac{1}{2}f_{\xi\eta}^i$

##### fine-tuning the optimization
Note that although, the aforementioned SRO's remain constant they contribute to the objective function
$\mathcal{O}(\sigma)$. One can avoid this by setting the {ref}`pair-weights parameter <input-param-pair-weights>`
($\tilde{p}_{\xi\eta}^i=0$ in Eq. {eq}`eqn:objective-actual`). Anyway the minimization will work.

We refine the above example in the following way 

```{code-block} yaml
---
lineno-start: 1
emphasize-lines: 18-23
caption: |
    Download the enhanced {download}`YAML file <examples/ti-al-b-n-prefactors-weights.yaml>` 
---
structure:
  supercell: [2, 2, 2]
  file: ti-n.cif
iterations: 5e6
shell_weights:
  2: 1.0
composition:
  B:
    N: 16
  N:
    N: 16
  Ti:
    Ti: 16
  Al:
    Ti: 16
prefactor_mode: mul
prefactors: 0.5
pair_weights:
  #   B    N    Al   Ti
  - [ 0.0, 0.5, 0.0, 0.0 ] # B
  - [ 0.5, 0.0, 0.0, 0.0 ] # N
  - [ 0.0, 0.0, 0.0, 0.5 ] # Al
  - [ 0.0, 0.0, 0.5, 0.0 ] # Ti
```

  - **Line 18-23:** set the pair-weight coefficients $\tilde{p}_{\xi\eta}^i$.
    - The main diagonal is set to zero, meaning we do not include same species pairs
    - We set parameters of species on different sublattices ($\alpha_{\text{B-Al}} = \alpha_{\text{N-Al}} = \alpha_{\text{B-Ti}} = \alpha_{\text{N-Ti}} = 0$) to zero. This will result in a more "*correct*" value for the objective function

Note, that this modification has no influence on the SRO parameters itself, but only on the value of the objective
function

## Using Python API
Of course, you can also directly use sqsgenerator directly from your Python interpreter. The package is designed in such
a way that all public function are gathered int the `sqsgenerator.public` module. Those which are needed to 
generate and analyze structure are forwarded to the *sqsgenerator* module itself and can be imported from there

Basically the API is build around two functions

  - {py:func}`sqsgenerator.public.sqs_optimize` - to perform SQS optimizations
  - {py:func}`sqsgenerator.public.sqs_analyse` - to compute objective function and SRO parameters

Both functions take a `dict` as their main input. The YAML inputs above are just a file-based representation of those 
settings.

### Introduction

To read a settings file and obtain a `dict`-like configuration use the {py:func}`sqsgenerator.public.read_settings_file`
function. The examples shown above, can be easily executed in the following way using a Python script:

```{code-block} python
# we use the first example shown in the CLI - How to -> re-w.first.yaml
from sqsgenerator import read_settings_file, sqs_optimize

configuration = read_settings_file('re-w.first.yaml')
results, timings = sqs_optimize(configuration)
```

(optimization-output)=
{py:func}`sqsgenerator.public.sqs_optimize` outputs a tuple of **two** values. Where the first one are the actual 
results, and the latter one are runtime information

  - `results` will contain a dictionary with integer keys. The integer key is the index of the permutation sequence.
     As this key is in decimal representation it might be a very long one. The value behind each key is a
     `dict` again, containing the following keys
    - `configuration`: a list of strings
    - `objective`: the value of the objective function
    - `parameters`: the SRO parameters as numpy array

### Again - $\text{Re}_{0.333}\text{W}_{0.667}$ - but from scratch
We now want to show how the {ref}`second example <example-two>` would look, like if it was built with Python functions

```{code-block}
from sqsgenerator import sqs_optimize

configuration = dict(
    structure=dict(file='b2.vasp', supercell=(3,3,3)),
    iterations=1e9,
    shell_weights={1: 1.0},
    composition=dict(Re=18, W=36)
)

results, timings = sqs_optimize(configuration)
```

### Exporting the generated structures

#### Construct the generated structures
By default, {py:func}`sqsgenerator.public.sqs_optimize` does not construct the {py:class}`Structure` objects from the 
generated configurations. You have to explicitly tell it using the `make_structures` keyword

Therefore, the last line in the previous example becomes

```{code-block} python
results, timings = sqs_optimize(configuration, make_structures=True)
```

This switch only affects post-processing, and adds a `structure` key to the `results` dictionary, which then becomes

```{code-block} python
{
    24002613167337: {
        'configuration': ['W', 'W', 'W', 'Re', 'W', 'Re', 'Re', 'W', 'W', 'W', 'Re', 'W', 'Re', 'Re', 'W', 'W', 'Re', 'Re', 'W', 'Re', 'Re', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'Re', 'W', 'W', 'W', 'W', 'Re', 'W', 'W', 'Re', 'W', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'W', 'Re', 'Re', 'W', 'W', 'W', 'W', 'W', 'W', 'W'],
        'objective': 5.551115123125783e-17,
        'parameters': array([[[5.00000000e-01, 5.55111512e-17], [5.55111512e-17, 5.00000000e-01]]]),
        'structure': Structure(W3ReWRe2W3ReWRe2W2Re2WRe2W7ReW4ReW2ReW2ReWReWReWRe2W7, len=54)
    }
}

```

````{admonition} Specify the type of the output structure using *structure_format* keyword
:class: tip, dropdown

In case you have set the `make_structures` keyword to `True` you additionally can specify the output type of the
structure objects using the `structure_format` keyword. Of course to use this you need 
[ase](https://wiki.fysik.dtu.dk/ase/) and/or [pymatgen](https://pymatgen.org/) installed to use this features.
You can pass three different values to `structure_format`:

  - **default** $\rightarrow$ {py:class}`sqsgenerator.public.Structure`
  - **ase** $\rightarrow$ {py:class}`ase.atoms.Atoms`
  - **pymatgen** $\rightarrow$ {py:class}`pymatgen.core.Structure`

````

#### Writing generated structures to file
In order to export the generated structures to files and/or archives using the 
{py:func}`sqsgenerator.public.export_structures` you need to set `make_structures=True` to advise the program to 
construct the structure. Moreover, `structure_format` must be set to `default` (which is anyway the default value).

Exporting the generated structures might look like that

```{code-block} python
from operator import itemgetter
from sqsgenerator import sqs_optimize, export_structures, read_settings_file

configuration = read_settings_file('sqs.yaml')
results, timings = sqs_optimize(configuration, make_structures=True)
export_structures(results, functor=itemgetter('structure'))
```

### Computing the SRO parameters $\alpha_{\xi\eta}^i$ and objective function $\mathcal{O}(\sigma)$ of existing structures

It is also possible to compute the SRO parameters of existing structure. Thus, the API exports the 
{py:func}`sqsgenerator.public.sqs_analyse`, which computes those quantities.

{py:func}`sqsgenerator.public.sqs_analyse` takes a dict-like configuration as well as an iterable of structures, which 
will be analysed. The output-format is exactly the same as for {py:func}`sqsgenerator.public.sqs_optimize` 
(see {ref}`above <optimization-output>`)

```{code-block} python

import numpy.testing
from operator import itemgetter
from sqsgenerator import sqs_optimize, read_settings_file, sqs_analyse

configuration = read_settings_file('sqs.yaml')
results, timings = sqs_optimize(configuration, make_structures=True, minimal=False, similar=True)  # same as --no-minimal --similar
structures = map(itemgetter('structure'), results.values())  # for this we need make_structures=True
analysed = sqs_analyse(structures, settings=configuration, append_structures=True)

for rank in results:
    # we check that we obatin the same results with sqs_analyse
    assert rank in analysed
    assert results[rank]['objective'] == analysed[rank]['objective']
    assert results[rank]['structure'] == analysed[rank]['structure']
    assert results[rank]['configuration'] == analysed[rank]['configuration']
    numpy.testing.assert_array_almost_equal(results[rank]['parameters'], analysed[rank]['parameters'])
    
```

### Other (maybe) useful examples

#### Conversion between structure types
*sqsgenerator*'s API export function to convert internal {py:class}`sqsgenerator.public.Structure` objects to types
employed by larger projects ([ase](https://wiki.fysik.dtu.dk/ase/) and [pymatgen](https://pymatgen.org/))

```{admonition} Packages must be available
:class: warning

In order to convert structure objects back and fourth you need to have this packages installed otherwise
sqsgenerator will raise a `FeatureError`
```

The compatibility functions are:
    
  - **pymatgen**: 
    - {py:func}`sqsgenerator.public.to_pymatgen_structure`
    - {py:func}`sqsgenerator.public.from_pymatgen_structure`
  - **ase**:
    - {py:func}`sqsgenerator.public.to_ase_atoms`
    - {py:func}`sqsgenerator.public.from_ase_atoms`

```{code-block} python
import numpy as np
import ase.atoms
import pymatgen.core
from sqsgenerator import to_pymatgen_structure, from_pymatgen_structure, to_ase_atoms, from_ase_atoms, Structure

fcc_al = Structure(4.05*np.eye(3), np.array([[0.0, 0.0, 0.0], [0.5, 0.5, 0.0], [0.0, 0.5, 0.5], [0.5, 0.0, 0.5]]), ['Al']*4)

fcc_al_ase = to_ase_atoms(fcc_al)
fcc_al_pymatgen = to_pymatgen_structure(fcc_al)

assert isinstance(fcc_al_ase, ase.atoms.Atoms)
assert isinstance(fcc_al_pymatgen, pymatgen.core.Structure)
assert fcc_al == from_ase_atoms(fcc_al_ase)
assert fcc_al == from_pymatgen_structure(fcc_al_pymatgen)
```
## Graceful exits

As the SQS optimization may require a large number of iterations, it is sometimes desirable to stop the process 
(e. g. because of time limits on HPC clusters). When sending a signal to *sqsgenerator* it does not crash but rather 
exit and write out the current state of the optimization.
*sqsgenerator*'s core routine installs a temporary signal *SIGINT* handler which replaces Pythons default 
`KeyboardInterrupt`. Thus while executing the optimization you can always interrupt it by hitting **Ctrl+C**. You should 
get a warning that the program was interrupted

```{code-block} bash
[warning]:do_pair_iterations::interrupt_message = "Received SIGINT/SIGTERM results may be incomplete"
/media/DATA/drive/projects/sqsgenerator-core/sqsgenerator/public.py:137: UserWarning: SIGINT received: SQS results may be incomplete
  warnings.warn('SIGINT received: SQS results may be incomplete')
```

In case of MPI parallel both *SIGINT* and *SIGTERM* handlers are overwritten. Therefore, if you run *sqsgenerator*
interactively using the `mpirun` command you can also gracefully terminate the process using **Ctrl+C**. How to 
terminate the program if executed with a queuing system behind, is documented in the 
[parallelization guide](parallelization.md).

## A note on the number of `iterations`

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
    for the current machine. The above command analyzes $10^5$ random configurations and 
    
  - Reduce the number of shells. This has two-fold advantage
    1. In contrast to old versions of *sqsgenerator*, the current implementations profit greatly from a decreased number
       of coordination shells. The actual speedup depends on the input structure but might be up to an order of 
       magnitude when compared to the default value (all shells are considered)
       ```{image} images/time_vs_shells.svg
       :alt: Estimated time vs. number of coordination shells
       :width: 67%
       :align: center
       ```
    3. The image size of the objective function is drastically reduced. In other words a lot of different structures are 
       mapped onto the same value of the objective function.
