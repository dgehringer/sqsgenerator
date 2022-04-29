# Input parameters

*sqsgenerator* uses a `dict`-like configuration to find all the value it needs. Thus it can it read its configuration from anything which can store Python `dict`s (e. g. `json`, `yaml` or `pickle`). However by default *sqsgenerator* expects an input file in **YAML** format. 

Each of the parameters below represents an entry in the **YAML** (or key in a `dict` if called directly from Python).

````{admonition} Input array interpretations
:class: note, dropdown

A closer look on the indices of the symbols Eq. {eq}`eqn:objective` reveals that those quantities need to be represented 
in a multidimensional manner. Those quantities are

  - $p_{\xi\eta}$ or $\tilde{p}^i_{\xi\eta}$: *{ref}`pair_weights <input-param-pair-weights>`*
  - $\tilde{\alpha}^i_{\xi\eta}$: *{ref}`target_objective <input-param-target-objective>`*
  - $f^i_{\xi\eta}$: *{ref}`prefactors <input-param-prefactors>`*
  
Although *sqsgenerator* accepts scalar values as input, there might be more complex use-cases where one wants to 
tweak the indiviual elements/coefficients (e. g. simoultaneous multi-sublattice optimization or partial ordering).

Thus, when specifiyng 2D and 3D input values for the aforementioned parameters, please make sure that
you have **read through** the {ref}`note on array input interpretation <input-param-target-objective>`
  
````

---

## Parameters

### `atol`
(input-param-atol)=

Absolute tolerance for calculating the default *{ref}`radii of the coordination shells <input-param-shell-distances>`* in $\mathrm{\mathring{A}}$. *{ref}`atol <input-param-atol>`* and *{ref}`atol <input-param-rtol>`* are used to determine if two numbers are close.

- **Required:** No
- **Default**: `1e-3`
- **Accepted:** positive floating point numbers (`float`)

### `rtol`
(input-param-rtol)=

relative tolerance for calculating the default *{ref}`radii of the coordination shells <input-param-shell-distances>`* in $\mathrm{\mathring{A}}$. *{ref}`atol <input-param-atol>`* and *{ref}`rtol <input-param-rtol>`* are used to determine if two numbers are close.

- **Required:** No
- **Default**: `1e-5`
- **Accepted:** positive floating point numbers (`float`)

````{admonition} What are **atol** and **rtol** used for?
:class: tip, dropdown

The `atol` and `rtol` parameters are used to determine if two floating point numbers are close. We use an implementation
 similar to Python [`math.isclose`](https://docs.python.org/3/library/math.html#math.isclose).

*{ref}`atol <input-param-atol>`* and *{ref}`rtol <input-param-rtol>`* parameters are used to compute the default values for the *{ref}`shell_distances <input-param-shell-distances>`* parameter.
Have a look on the *{ref}`shell_distances <input-param-shell-distances>`* parameter, by entering:

  ```{code-block} bash
  sqsgenerator params show input.yaml --param shell_distances
  ```
In case you get some distances which are really close e. g. 4.12345 and 4.12346 it is maybe a good idea to increase 
*{ref}`atol <input-param-atol>`*  and/or *{ref}`rtol <input-param-rtol>`* such that *sqsgenerator* groups them into the same coordination shell

Changing *{ref}`atol <input-param-atol>`*  and/or *{ref}`rtol <input-param-rtol>`* parameters will change the number and radii of the computed coordination shells
````

### `max_output_configurations`
(input-param-max-output-configurations)=
The maximum number of output configurations. 

- **Required:** No
- **Default:** 10
- **Accepted:** positive finite integer number (`int`)

```{admonition} Behaviour when running with MPI parallelization
:class: note, dropdown

In case the code runs with MPI parallelization, **each MPI rank** will generate at most `max_output_configurations` which are gathered at the head rank.
```

### `composition` 
(input-param-composition)=

The composition of the output configuration, defined as an dictionary.  Keys are symbols of chemical elements, 
whereas values are the number of atoms of the corresponding species. The number in the dict-values or the length of the 
specified **match** the number of specified positions on the sublattice. See *{ref}`which <input-param-which>`* input
parameter. The composition parameter might be also used to pin atomic species on current sublattices

- **Required:** Yes
- **Accepted:**
  - a dictionary with chemical symbols as keys and numbers as values (`dict[str, int]`)
  - a dictionary with chemical symbols as keys and dict as values (`dict[str, dict[str, int]]`)

```{note}
 - The sum of the atoms distributed must **exactly** match the number of positions on the lattice
   In combination with *{ref}`which <input-param-which>`* the number must match, the amount of selected sites
 - If you explicitly pin atomic species on certain sublattices (see examples below) you have to specify it for all
   - If you do that the number of distributed atoms must match the number of lattice positions on the specified sublattice
```

#### Examples
- Ternary alloy, consisting of 54 atoms ($\text{Ti}_{18}\text{Al}_{18}\text{Mo}_{18}$)

    ```{code-block} yaml
    composition:
      Ti: 18
      Al: 18
      Mo: 18
    ```

- *fcc*-Aluminum cell,  64 atoms, randomly distribute  8 vacancies

    ```{code-block} yaml
    composition:
      Al: 56
      0: 8
    ```

- consider a $\text{Ti}_{0.5}\text{N}_{0.5}$ supercell with 64 atoms. 
  Let's $(\text{Ti}_{0.25}\text{Al}_{0.25})(\text{B}_{0.25}\text{N}_{0.25})$ create an alloy from this cell.
  The input structure provides us with 32 lattice position of Ti and 32 positions of N. However boron will sit only on 
  the nitrogen sites, while aluminium will most likely occupy only titanium sites. Therefore, we can constrain how 
  *sqsgenerator* will distribute the atoms 

    ```{code-block} yaml
    composition: 
      Ti:
        Ti: 16 # distribute 16 Ti atoms on the Ti occupied lattice positions of the input structure
      Al:
        Ti: 16 # occupy remaining Ti sublattice positions with 16 aluminum atoms
      N: # occupy the initial 32 nitrogen sites with 16 B and 16 N atoms
        N: 16
      B:
        N: 16      
    ```

### `which`
(input-param-which)=
Used to select a sublattice (collection of lattice sites) from the specified input structure. Note that the number of atoms in the *{ref}`composition <input-param-composition>`* parameter has to sum up to the number of selected lattice positions

- **Required:** No
- **Default:** *all*
- **Accepted:**
  - either *all* or a chemical symbol specified in the input structure (`str`)
  - list of indices to choose from the input structure (`list[int]`)

```{note}
The sum of the atoms distributed must **exactly** match the number of **selected** lattice positions. 
```

#### Examples
- Ternary alloy, 54 atoms, create ($\text{Ti}_{18}\text{Al}_{18}\text{Mo}_{18}$)
  
    ```{code-block} yaml
    which: all
    composition:
      Ti: 18
      Al: 18
      Mo: 18
    ```
    
- *rock-salt* TiN (B1),  64 atoms, randomly distribute B and N on the N sublattice $\text{Ti}_{32}(\text{B}_{16}\text{N}_{16}) = \text{Ti}(\text{B}_{0.5}\text{N}_{0.5})$
  
    ```{code-block} yaml
    which: N
    composition:
      N: 16
      B: 16
    ```
    
- *rock-salt* TiN (B1),  64 atoms, randomly distribute Al, V and Ti on the Ti sublattice $(\text{Ti}_{16}\text{Al}_{8}\text{V}_{8})\text{N}_{32} = (\text{Ti}_{0.5}\text{Al}_{0.25}\text{V}_{0.25})\text{N}$
  
    ```{code-block} yaml
    which: Ti
    composition:
      Ti: 16
      Al: 8
      V: 8
    ```
    
- select all **even** sites from your structure, 16 atoms, using a index, list and distribute W, Ta and Mo on those sites
  
    ```{code-block} yaml
    which: [0, 2, 4, 6, 8, 10, 12, 14]
    composition:
      W: 3
      Ta: 3
      Mo: 2
    ```

### `structure`
(input-param-structure)=
the structure where *sqsgenerator* will operate on *{ref}`which <input-param-which>`* will select the sites from the specified structure. The coordinates must be supplied in **fractional** style. It can be specified by supplying a filename or directly as a dictionary

- **Required:** Yes
- **Accepted:**
  - dictionary with a `file` key (`dict`)
  - dictionary with a `lattice`, `coords` and `species` key (`dict`)

```{note}
  - If a filename is specified, and `ase` is available *sqsgenerator* will automatically use it to load the structure using [`ase.io.read`](https://wiki.fysik.dtu.dk/ase/ase/io/io.html#ase.io.read). Alternatively it will fall back to `pymatgen` ( [`pymatgen.core.IStructure.from_file`](https://pymatgen.org/pymatgen.core.structure.html#pymatgen.core.structure.IStructure.from_file)). If both packages are not available it will raise an `FeatureError`.
  - You can explicitly instruct to use one of the packages by settings `structure.reader` to either *ase* or *pymatgen*
  - You can pass custom arguments to the reader function ( [`ase.io.read`](https://wiki.fysik.dtu.dk/ase/ase/io/io.html#ase.io.read) or [`pymatgen.core.IStructure.from_file`](https://pymatgen.org/pymatgen.core.structure.html#pymatgen.core.structure.IStructure.from_file)) by specifying `structure.args` (last example)
```

#### Examples

- directly specify $\text{CsCl}$ (B2) structure in the input file

    ```{code-block} yaml
    structure:  
      lattice:
        - [4.123, 0.0, 0.0]
        - [0.0, 4.123, 0.0]
        - [0.0, 0.0, 4.123]
      coords: # put fractional coordinates here -> not cartesian
        - [0.0, 0.0, 0.0]
        - [0.5, 0.5, 0.5]
      species:
        - Cs
        - Cl
    ```
    Please note that for each entry in `coords` there must be an corresponding species specified in the `species` list
    
-  specify a file (must be readable by `ase.io.read` , fallback to `pymatgen` if `ase` is not present)
   
    ```{code-block} yaml
    structure:
      file: cs-cl.vasp # POSCAR/CONTCAR format
    ```
    
- specify a file and explicitly set a reader for reading the structure file
  
    ```{code-block} yaml
    structure:
       file: cs-cl.cif
       reader: pymatgen # use pymatgen to read the CIF file
    ```
  
- specify read a file and  pass arguments to the reading package. E. g. read las configuration from a MD-trajectory
  
    ```{code-block} yaml
    structure:
      file: md.traj
      reader: ase
      args:
        index: -1
    ```
    if `args` is present in will be unpacked (`**`) into `ase.io.read`


### `structure.supercell`

Instructs *sqsgenerator* to create a supercell of the specified *{ref}`structure <input-param-structure>`*

- **Required:** No
- **Accepted:** a list/tuple of positive integer number of length 3 (`tuple[int]`)

#### Examples

- Create a $3\times3\times3$ supercell of the $\text{CsCl}$ (B2) structure

    ```{code-block} yaml
    structure:
      supercell: [3, 3, 3]
      lattice:
        - [4.123, 0.0, 0.0]
        - [0.0, 4.123, 0.0]
        - [0.0, 0.0, 4.123]
      coords: # put fractional coordinates here -> not cartesian
        - [0.0, 0.0, 0.0]
        - [0.5, 0.5, 0.5]
      species:
        - Cs
        - Cl
    ```

- Create a  $3\times3\times3$ supercell of a structure file

    ```{code-block} yaml
    structure:
      supercell:
        - 3
        - 3
        - 3
      file: cs-cl.cif

### `mode`
(input-param-mode)=

The iteration mode specifies how new structures are generated. 

- *random*: the configuration will be shuffled randomly
- *systematic*: will instruct the code generate configurations in lexicographical order and to scan the **complete configurational space**. In case *systematic* is specified the *{ref}`iterations <input-param-iterations>`* parameter will be ignored, since the number of permutations is predefined. Therefore, for a system with $N$ atoms with $M$ species, will lead to

$$
N_{\text{iterations}} = \dfrac{N!}{\prod_m^M N_m!} \quad \text{where} \quad \sum_m^M N_m = N
$$

- **Required:** No
- **Default:** *random*
- **Accepted:** *random* or *systematic* (`str`)

````{admonition} How long will you calculation run?
:class: tip, dropdown

- In case you use the *systematic* mode make sure that the total number of configurations to check is a number a computer can check in finite times. To compute the total number of iterations you can use:
```{code-block} bash
sqsgenerator compute total-permutations
```
- *sqsgenerator* also allows you to guess how long it might take to check a certain number of iterations:
```{code-block} bash
sqsgenerator compute estimated-time
```
````

### `iterations`
(input-param-iterations)=

Number of configurations to check. This parameter is ignored if *{ref}`mode <input-param-mode>`* was set to *systematic*

- **Required:** No
- **Default:** $10^5$ if *{ref}`mode <input-param-mode>`* is *random*
- **Accepted:** a positive integer number (`int`)

### `shell_distances`
(input-param-shell-distances)=
the radii of the coordination shells in Angstrom. All lattice positions will be binned into the specified coordination 
shells. The default distances are computed in the following way:

```{code-block} python
---
caption: |
    This is just a Python implementation which demonstrates what is happening
---
shell_distances = []

# In the following atol and rtol represent the input parameters of atol and rtol

def get_closest_shell(r_ij: float) -> int:
    predicate = functools.partial(math.isclose, r_ij, abs_tol=atol, rel_tol=rtol)
    return next(filter(predicate, shell_distances), None)

# Let Rij be the distance matrix of the input structure as np.ndarray
for r_ij in sorted(Rij.flat):
    closest_shell = get_closest_shell(r_ij)
    if closest_shell: # we compute a roling average, to estimate a mean value
        shell_distances[closest_shell] = (shell_distances[closest_shell] + r_ij) / 2.0
    else: # no value similar to r_ij exists, we create a new shell
        shell_distances.append(r_ij)
    shell_distances = sorted(shell_distances)
    
```


- **Required:** No
- **Default:** automatically determined by *sqsgenerator*
- **Accepted:** a list of positive floating point numbers (`list[float]`)

````{hint}
You can have a look at the the computed shell distances, and check if they are fine using:
```{code-block} bash
sqsgenerator params show input.yaml -p shell_distances
```
````

### `shell_weights`
(input-param-shell-weights)=

accounts for the fact that coodination shells which are farther away are less important. This parameter also determines 
**which** shells should be taken into account. The `shell_weights` are a mapping (dictionary). It assigns the 
**shell index** it corresponding shell weight $w^i$. The keys represent the indices of the calculated shell distances 
computed or specified by the *{ref}`shell_distances <input-param-shell-distances>`* parameter. Its values correspond 
to $w^i$ {eq}`eqn:objective` in the objective function.

- **Required**: No
- **Default**: $\frac{1}{i}$ where $i$ is the index of the coordination shell. Automatically determined by *sqsgenerator*
- **Accepted:** a dictionary where keys are the shell indices and the values $w^i$ parameters (`dict[int, float]`)

````{admonition} Further information
:class: note, dropdown

- If nothing is specified **all available** coordination shells are used
- You can improve the performace of *sqsgenerator* by neglecting some coordination shells
- If you specify a shell index which does not exist a `BadSettings` error will be raised
- Indices which are not specified explicitly specified are not considered in the optimization
- You can have a look on the generated weights by checking
```{code-block} bash
sqsgenerator params show input.yaml -p shell_weights
```
````

#### Examples
To consider all coordination shells, simply do not specify any value

- Only use the first coordination shell
  
  ```{code-block} yaml
  shell_weights:
    1: 1.0
  ```

- Use first and second coodination shell
  
  ```{code-block} yaml
  shell_weights:
    1: 1.0 # this are the default values anyway
    2: 0.5
  ```
  
### `pair_weights`
(input-param-pair-weights)=

thr "*pair weights*" $p_{\xi\eta}$/$\tilde{p}_{\xi\eta}^i$  {eq}`eqn:objective-actual` used to differentiate bonds between atomic species.
Note that *sqsgenerator* sorts the atomic species interally in ascending order by their ordinal number.
Please refer to the *{ref}`target_objective <input-param-target-objective>`* parameter documentation for further details regarding the internal reordering.

The default value is a hollow matrix, which is multiplied with the corresponding shell weight

$$
  p_{\xi\eta} = \frac{1}{2}\left(\mathbf{J}_N - \mathbf{I}_N \right)
$$ (eqn:parameter-weight-single-shell)

where $N=N_{\text{species}}$, $\mathbf{J}_N$ the matrix full of ones and $\mathbf{I}_N$ the identity matrix.
Using this formalism the default value for $\tilde{p}_{\xi\eta}^i$ is calculates according to Eq. {eq}`eqn:parameter-weight-efficient` and
{eq}`eqn:parameter-weight-single-shell` as

$$
  \tilde{p}_{\xi\eta}^i = w^i p_{\xi\eta}  = \frac{1}{2}w_i\left(\mathbf{J}_N - \mathbf{I}_N \right)
$$ (eqn:parameter-weight-default)

where $w^i$ is the *{ref}`shell_weight <input-param-shell-weights>`* of the i$^\text{th}$ coordination shell. If a 2D input or any of the of the sub-array 
in case of a 3D input array is not symmetric a `BadSettings` exception is raised.

- **Required:** No
- **Default:** an array as described in Eq. {eq}`eqn:parameter-weight-default` shape $\left(N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$
- **Accepted:**  
  - a 2D matrix of shape $\left( N_{\text{species}}, N_{\text{species}} \right)$. The input is interpreted as $p_{\xi\eta}$ and will be stacked along the first dimensions and multiplied with $w_i$ to generate a shape of $N_{\text{shells}}$ times to generate the $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$ array (`np.ndarray`)
  - a 3D array of shape $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$. The input is interpreted as $\tilde{p}_{\xi\eta}^i$ (`np.ndarray`)

### `target_objective`
(input-param-target-objective)=

the target objective $\tilde{\alpha}^i_{\eta\xi}$ {eq}`eqn:objective`, which the SRO parameters {eq}`eqn:wc-sro-multi` are minimzed against. It is an array of three-dimensions of shape $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$. By passing custom values you can fine-tune the individual SRO paramters.

- **Required:** No
- **Default:** an array of **zeros** of shape $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$
- **Accepted:**
  - a single scalar value. An array filled with the scalar value of shape $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$ will be created (`float`)
  - a 2D matrix of shape $\left( N_{\text{species}}, N_{\text{species}} \right)$ the matrix will be stacked along the first dimension $N_{\text{shells}}$ times to generate the $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$ array (`np.ndarray`)
  - a 3D array of shape $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$ (`np.ndarray`)

(note-on-array-input-interpretation)=
````{admonition} A note on array input interpretation
:class: note, dropdown

-  because of $\tilde{\alpha}^i_{\xi\eta}= \tilde{\alpha}^i_{\eta\xi}$ the `target_objective` is a **symmetric** quantity. Thus in case an  $\tilde{\alpha}_{\eta\xi}^{i,T} \neq \tilde{\alpha}_{\xi\eta}$ an `BadSettings` error is raised
-  the atomic species specified in `strcuture` by their ordinal number in **ascending order** 
    - In case of the $\text{CsCl}$ the actual ordering is $\text{Cl}(Z=17), \text{Cs}(Z=55)$.
    ```{math}
    \boldsymbol{\tilde{\alpha}}^i = \left[
    \begin{array}{cc}
    \tilde{\alpha}_{\text{Cl-Cl}} & \tilde{\alpha}_{\text{Cl-Cs}} \\
    \tilde{\alpha}_{\text{Cs-Cl}} & \tilde{\alpha}_{\text{Cs-Cs}}
    \end{array}
    \right]
    ```
    - In case of the $\text{TiAlMo}$ the actual ordering is $\text{Al}(Z=13), \text{Ti}(Z=22), \text{Mo}(Z=42)$.
    ```{math}
    \boldsymbol{\tilde{\alpha}}^i = \left[
    \begin{array}{ccc}
    \tilde{\alpha}_{\text{Al-Al}} & \tilde{\alpha}_{\text{Al-Ti}} & \tilde{\alpha}_{\text{Al-Mo}} \\
    \tilde{\alpha}_{\text{Ti-Al}} & \tilde{\alpha}_{\text{Ti-Ti}} & \tilde{\alpha}_{\text{Ti-Mo}} \\
    \tilde{\alpha}_{\text{Mo-Al}} & \tilde{\alpha}_{\text{Mo-Ti}} & \tilde{\alpha}_{\text{Mo-Mo}} \\
    \end{array}
    \right]
    ```
````

#### Examples
- distribute everything randomly
  
  ```{code-block} yaml
  target_objective: 0 # this is the default behaviour
  ```
- search for a clustered $\text{CsCl}$ structure

  ```{code-block} yaml
  target_objective: 1 # which is equivalent to 
  target_objective:
    - [1, 1]
    - [1, 1]
  ``` 
  
- custom settings for a ternary alloy (unknown use case :smile: )

  ```{code-block} yaml
  target_objective:
    - [ 1, -1, 0]
    - [-1,  1, 0]
    - [ 0,  0, 1]
  ``` 

### `prefactors`
(input-param-prefactors)=

The bond prefactors $f_{\xi\eta}^i$ as defined in Eq. {eq}`eqn:prefactors`. The input representation and options are the
same as for the *{ref}`target_objective <input-param-target-objective>`* parameter. The 
{ref}`prefactor_mode <input-param-prefactor-mode>` parameter determines whether the default values are overridden or 
modified. One can think of the prefactors $f^i_{\xi\eta}$ as the reciprocal value of the expected number $\xi - \eta$ 
pairs in the $i^{\text{th}}$ coordination shell

- **Required:** No
- **Default:** an array of **zeros** of shape $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$
- **Accepted:**
  - a single scalar value. An array filled with the scalar value of shape $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$ will be created (`float`)
  - a 2D matrix of shape $\left( N_{\text{species}}, N_{\text{species}} \right)$ the matrix will be stacked along the first dimension $N_{\text{shells}}$ times to generate the $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$ array (`np.ndarray`)
  - a 3D array of shape $\left( N_{\text{shells}}, N_{\text{species}}, N_{\text{species}} \right)$ (`np.ndarray`)


### `prefactor_mode`
(input-param-prefactor-mode)=

Specifies how the *{ref}`prefactors <input-param-prefactors>`* parameter is interpreted

- *set*  uses the values from *{ref}`prefactors <input-param-prefactors>`* and interprets them directly as $f^i_{\xi\eta}$
- *mul* multiplies the input array **element-wise** with the default values of *{ref}`prefactors <input-param-prefactors>`*
  and uses the result as $f^i_{\xi\eta}$

- **Required:** No
- **Default:** *set*
- **Accepted:** *set* or *mul* (`str`)

 ### `threads_per_rank`

number of threads should be used on each rank. The value(s) is (are) passed on to 
[*omp_set_num_threads*](https://www.openmp.org/spec-html/5.0/openmpsu110.html). If the version of *sqsgenerator* 
**is not capable**  MPI parallelism, a single value is needed. If *sqsgenerator* was called within a MPI runtime, 
an entry must be present for each rank. In case OpenMP schedules a different number of threads, than specified in 
`threads_per_rank` the workload will be redistributed automatically. Negative values represent a call to 
[*omp_get_max_threads*](https://www.openmp.org/spec-html/5.0/openmpsu112.html) (as many threads as possible). Further 
reading can be found in the [parallelization guide](parallelization.md).

- **Required:** No
- **Default:** `[-1]` if there is no MPI support. Otherwise `[-1]*N` where `N` is the number of MPI ranks
- **Accepts:** a list of integers number (`list[int]`)