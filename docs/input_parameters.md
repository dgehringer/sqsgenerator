# Input parameters

`sqsgenerator` uses a `dict`-like configuration to find all the value it needs. Thus it can it read its configuration from anything which can store Python `dict`s (e. g. `json`, `yaml` or `pickle`). However by default `sqsgenerator` expects an input file in **YAML** format. 

Each of the parameters below represents an entry in the **YAML** (or key in a `dict` if called directly from Python).

(The order corresponds to what is parsed in `sqsgenerator.settings.py`)

- `atol` - absolute tolerance for calculating the default distances of the coordination shells (unit $\text{\AA}$). `atol` and `rtol` are used to determine if two numbers are close.

  **Required:** No

  **Default**: `1e-3`
  **Accepted:** positive floating point numbers

* `rtol` - relative tolerance for calculating the default distances of the coordination shells (unit $\text{\AA}$​​​​). `atol` and `rtol` are used to determine if two numbers are close.

  **Required:** No

  **Default**: `1e-5`

  **Accepted:** positive floating point numbers
  **Hint:** Have a look on the `shell_distances` parameter, by entering

  ```bash
  sqsgenerator params show input.yaml --param shell_distances
  ```

  