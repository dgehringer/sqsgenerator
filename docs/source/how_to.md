
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
default, the program assumes the configuration to be stored in a YAML file.

YAML is easy to read and write by humans. On this tutorial site we will focus on setting up proper `sqs.yaml` input 
files.

Most of the CLI commands which require an input settings file. However, specifying a file is always optional, since 
*sqsgenerator* will always look for a default file name `sqs.yaml` in the **current directory**.

*sqsgenerator* can also read more formats, which can store Python `dict`s, such as **JSON** and **pickle**. Therefore, 
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
Otherwise, it will modify the passed filename e. g. `re-w.first.yaml` $\rightarrow$ `re-w.first.result.yaml`


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

### Analyse existing structures
Sometimes it is desirable to compute the SRO parameters ($\alpha^i_{\xi\eta}$) for an exiting arrangement of atoms 
rather than to generate a new one. To analyse existing structures *sqsgenerator* provides you with the "*analyse*" command.

#### Restore $\alpha^i_{\xi\eta}$ from structure files

**Note:** This example only worke with `pymatgen` or `ase` installed

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
        

#### Counting pairs in coordination shells using the `analyse` command

*sqsgenerator* can also compute the number of bonds in existing structures, by tweaking parameters
for the `analyse` command properly.

A closer look on Eq. {eq}`eqn:sro-modified` reveals, by setting the {ref}`prefactors <input-param-prefactors>`
$f^i_{\xi\eta} = 1$ the SRO parameters become $\alpha^i_{\xi\eta} = 1 - N^i_{\xi\eta}$. Hence by modifying
*settings.yaml* file to

```{code-block} yaml
---
lineno-start: 1
emphasize-lines: 4-5
---
shell_weights:
  1: 1.0
  2: 0.5
prefactor_mode: set
prefactors: 1
```

- **Line 4:** explicitly overrides the values of $f^{i}_{\xi\eta}$ with those provided in the file
- **Line 5:** set $f^i_{\xi\eta}$ to 1

To obtain the number of $\xi - \eta$ pair we have to compute $N^i_{\xi\eta} = 1 - \alpha^i_{\xi}$
*sqsgenerator* support also other output formats than printing it to the console. Hence, we want to illustrate
how *sqsgenerator*'s CLI can be used directly in combination with Python without using the Python API

```{code-block} python
import os
import yaml
import pprint
import numpy as np

# analyse the structure and export results in YAML format(--output-format/-of yaml)
yaml_output = os.popen('sqsgen analyse *.cif -s settings.yaml -of yaml')
results = yaml.safe_load(yaml_output)

# loop over output results
for analysed_file, configurations in results.items():
    for rank, results in configurations.items():
        # actually compute N = 1.0 - alpha
        results['bonds'] = 1.0 - np.array(results.get('parameters'))
        
pprint.pprint(results)
```


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
(e.g. because of time limits on HPC clusters). When sending a signal to *sqsgenerator* it does not crash but rather 
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
interactively using the `mpirun` command you can also gracefully terminate the process using **Ctrl+C**.

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

  - Maybe you have knowledge about the system: E. g certain species are restricted on 
    different sub-lattices.

  - Check how long it would take to compute your current settings
  
    ```{code-block} bash
    sqsgen compute estimated-time
    ```

    You can tune the number of permutations to a computing time you can afford. The above command gives only an estimate
    for the current machine. The above command analyzes $10^5$ random configurations and extrapolates it to the desired
    number of iterations. However, this value should be seen as an **upper bound**, as cycle times 
    are slightly reduced for large number of iterations
    
  - Reduce the number of shells. This has two-fold advantage:
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


### A simple convergence-test 
    
```{code-block} python
---
lineno-start: 1
emphasize-lines: 7,11,15-19,23,24,26,32,34
caption: |
    Script to perform a "*convergence test*" for the parameters $N^{\mathrm{shells}}$ and $N^{\mathrm{iterations}}$.
---

from matplotlib import pyplot as plt
from sqsgenerator import sqs_optimize
from operator import itemgetter as item
from math import isclose, factorial as f, log10

# compute size of configurational space
conf_space_size = f(36)/(f(24)*f(12))

NSHELLS=7  # max number of shells
MIN_MAGNITUDE=4  # minimum number of iterations
MAX_MAGNITUDE=int(log10(conf_space_size))  # maximum number of iterations
SAMPLES=int(10**MIN_MAGNITUDE) # maximum number of structures

# 36 atoms hcp with 12 Re and 24 W atoms
settings = dict(
    structure=dict(file='hcp.vasp', supercell=[2, 2, 3]),
    composition=dict(W=12, Re=24),
    max_output_configurations=SAMPLES,
)

test_results = dict()
for shells in range(1, NSHELLS+1):
    settings['mode'] = 'systematic'  # perform exhaustive search
    settings['shell_weights'] = {i: 1.0/i for i in range(1, shells + 1) }
    # compute the best value of the objective function by exhaustive enumeration
    sys_results, *_ = sqs_optimize(settings, fields=('objective',))
    best_objective = min(sys_results.values(), key=item('objective')).get('objective') 
    test_results[shells] = []  # create a list where we store 
    for mag in range(MIN_MAGNITUDE, MAX_MAGNITUDE+1):
        settings['mode'] = 'random'
        settings['iterations'] = int(10**mag)
        results, *_ = sqs_optimize(settings, minimal=False, similar=True, fields=('objective',))
        # compute percentage of structures that exhibit minimal objective
        percent = sum(isclose(r.get('objective'), best_objective) for r in results.values()) / len(results) * 100
        test_results[shells].append((mag, percent))

def transpose(it) -> zip:
    return zip(*it)

# visualize the data
for shells, data in sorted(test_results.items(), key=item(0)):
    plt.plot(*transpose(data), marker='o', label=f'$S={shells}$')
plt.axvline(log10(conf_space_size), color='k', label='exhaustive')
plt.xlabel(r'$\log(N^{iter})$')
plt.ylabel(r'$\frac{N^{best}}{N^{total}} [\%]$')
plt.legend()
plt.savefig('convergence_test.pdf')
```

  - **Line 7:** compute the total number of iterations for the exhaustive search according to Eq.~{eq}`eqn:multinomial`.
    In the present case $N^{\text{iterations}} = \frac{36!}{12!24!} \approx 1.25 \cdot 10^9$