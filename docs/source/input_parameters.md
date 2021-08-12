# Input parameters

`sqsgenerator` uses a `dict`-like configuration to find all the value it needs. Thus it can it read its configuration from anything which can store Python `dict`s (e. g. `json`, `yaml` or `pickle`). However by default `sqsgenerator` expects an input file in **YAML** format. 

Each of the parameters below represents an entry in the **YAML** (or key in a `dict` if called directly from Python).

(The order corresponds to what is parsed in `sqsgenerator.settings.py`)

---

## Parameters

### `atol`

Absolute tolerance for calculating the default distances of the coordination shells (unit $\text{\AA}$). `atol` and `rtol` are used to determine if two numbers are close.

- **Required:** No
- **Default**: `1e-3`
- **Accepted:** positive floating point numbers (`float`)

### `rtol`

relative tolerance for calculating the default distances of the coordination shells (unit $\text{\AA}$). `atol` and `rtol` are used to determine if two numbers are close.

- **Required:** No
- **Default**: `1e-5`
- **Accepted:** positive floating point numbers (`float`)

````{hint}
Have a look on the `shell_distances` parameter, by entering:

  ```{code-block} bash
  sqsgenerator params show input.yaml --param shell_distances
  ```
In case you get some distances which are really close e. g. 4.12345 and 4.12346 it is maybe a good idea to increase `rtol` and/or `atol` such that `sqsgenerator` groups them into the same coordination shell
````

### `max_output_configurations`

The maximum number of output configurations. 

- **Required:** No
- **Default:** 10
- **Accepted:** positive finite integer number (`int`)

```{note}
In case the code runs with MPI parallelization, **each MPI rank** will generate at most `max_output_configurations` which are gathered at the head rank.
```

### `composition` 

The composition of the output configuration, defined as an dictionary.  Keys are symbols of chemical elements, whereas values are the number of atoms of the corresponding species. 

- **Required:** Yes
- **Accepted:** a dictionary with chemical symbols as keys (`dict[str, int]`) 

```{note}
The sum of the atoms distributed must **exactly** match the number of positions on the lattice 
```

#### Examples
- Ternary alloy, consisting of 54 atoms ($\text{Ti}_{18}\text{Al}_{18}\text{Mo}_{18}$)

    ```yaml
    composition:
    Ti: 18
    Al: 18
    Mo: 18
    ```

- *fcc*-Aluminum cell,  64 atoms, randomly distribute  8 vacancies

    ```yaml
    composition:
    Al: 56
    0: 8
    ```

### `composition.which`

Used to select a sublattice (collection of lattice sites) from the specified input structure. Note that the number of atoms in the `composition` paramter has to sum up to the number of selected lattice positions

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
    
    ```yaml
    composition:
      which: all
      Ti: 18
      Al: 18
      Mo: 18
    ```
    
- *rock-salt* TiN (B1),  64 atoms, randomly distribute B and N on the N sublattice $\text{Ti}_{32}(\text{B}_{16}\text{N}_{16}) = \text{Ti}(\text{B}_{0.5}\text{N}_{0.5})$
    
    ```yaml
    composition:
      which: N
      N: 16
      B: 16
    ```
    
- *rock-salt* TiN (B1),  64 atoms, randomly distribute Al, V and Ti on the Ti sublattice $(\text{Ti}_{16}\text{Al}_{8}\text{V}_{8})\text{N}_{32} = (\text{Ti}_{0.5}\text{Al}_{0.25}\text{V}_{0.25})\text{N}$
    
    ```yaml
    composition:
      which: Ti
      Ti: 16
      Al: 8
      V: 8
    ```
    
- select all **even** sites from your structure, 16 atoms, using a index, list and distribute W, Ta and Mo on those sites
    
    ```yaml
    composition:
      which: [0, 2, 4, 6, 8, 10, 12, 14]
      W: 3
      Ta: 3
      Mo: 2
    ```

### `structure`

the structure where `sqsgenerator` will operate on. `composition.which` will select the sites from the specified structure. The coordinates must be supplied in **fractional** style. It can be specified by supplying a filename or directly as a dictionary

- **Required:** Yes
- **Accepted:**
  - dictionary with a `file` key (`dict`)
  - dictionary with a `lattice`, `coords` and `species` key (`dict`)

```{note}
  - If a filename is specified, and `ase` is available `sqsgenerator` will automatically use it to load the structure using [`ase.io.read`](https://wiki.fysik.dtu.dk/ase/ase/io/io.html#ase.io.read). Alternatively it will fall back to `pymatgen` ( [`pymatgen.core.IStructure.from_file`](https://pymatgen.org/pymatgen.core.structure.html#pymatgen.core.structure.IStructure.from_file)). If both packages are not available it will raise an `FeatureError`.
  - You can explicitly instruct to use one of the packages by settings `structure.reader` to either *ase* or *pymatgen*
  - You can pass custom arguments to the reader function ( [`ase.io.read`](https://wiki.fysik.dtu.dk/ase/ase/io/io.html#ase.io.read) or [`pymatgen.core.IStructure.from_file`](https://pymatgen.org/pymatgen.core.structure.html#pymatgen.core.structure.IStructure.from_file)) by specifying `structure.args` (last example)
```

#### Examples

- directly specify CsCl (B2) structure in the input file

    ```yaml
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
    
    ```yaml
    structure:
      file: cs-cl.vasp # POSCAR/CONTCAR format
    ```
    
- specify a file and explicitly set a reader for reading the structure file
    
    ```yaml
    structure:
       file: cs-cl.cif
       reader: pymatgen # use pymatgen to read the CIF file
    ```
  
- specify read a file and  pass arguments to the reading package. E. g. read las configuration from a MD-trajectory
    
    ```yaml
    structure:
      file: md.traj
      reader: ase
      args:
        index: -1
    ```
    if `args` is present in will be unpacked (`**`) into `ase.io.read`


### `structure.supercell`

Instructs `sqsgenerator` to create a supercell of the the specified structure

- **Required:** No
- **Accepted:** a list/tuple of positive integer number of length 3 (`tuple[int]`)

#### Examples

- Create a $3\times3\times3$ supercell of the CsCl (B2) structure

    ```yaml
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

    ```yaml
    structure:
      supercell:
        - 3
        - 3
        - 3
      file: cs-cl.cif

### `mode`

The iteration mode specifies how new structures are generated. 

- *random* the configuration will be shuffled randomly

- *systematic* will instruct the code generate configurations in lexicographical order and to scan the **complete configurational space**. In case *systematic* is specified the `iterations` parameter will be ignored, since the number of permutations is predefined. Therefore for a system with $N$ atoms with $M$ species, will lead to

  $$
  N_{\text{iterations}} = \dfrac{N!}{\prod_m^M N_m!} \quad \text{where} \quad \sum_m^M N_m = N
  $$
  
  iterations.

- **Required:** No
- **Default:** *random*
- **Accepted:** *random* or *systematic* (`str`)

````{hint}
- In case you use the *systematic* make sure that the total number of configurations to check is a number a computer can check in finite times. To compute the total number of permutations you can use:
```{code-block} bash
sqsgenerator compute total-permutations
```
- `sqsgenerator` also allows you to guess how long it might take to check a certain number of iterations:
```{code-block} bash
sqsgenerator compute estimated-time
```
````