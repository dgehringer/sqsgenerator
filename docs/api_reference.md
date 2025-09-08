
# API Reference


 ## important functions


```{eval-rst}

.. autofunction:: sqsgenerator.parse_config

.. autofunction:: sqsgenerator.optimize

.. autofunction:: sqsgenerator.load_result_pack

.. autofunction:: sqsgenerator.write

.. autofunction:: sqsgenerator.read

```

## conversion functions

```{eval-rst}

.. autofunction:: sqsgenerator.to_ase

.. autofunction:: sqsgenerator.from_ase

.. autofunction:: sqsgenerator.to_pymatgen

.. autofunction:: sqsgenerator.from_pymatgen

```


## classes

For each of the classes with a *Double* suffix there exists also a *Float* variant. The Float variants are not documented here to reduce clutter, but they behave exactly the same as their Double counterparts, just with lower precision.


```{eval-rst}

.. autoclass:: sqsgenerator.core.Structure

.. autoclass:: sqsgenerator.core.StructureDouble
   :members:
   :undoc-members:
   :inherited-members:


.. autoclass:: sqsgenerator.core.SqsResultPack

.. autoclass:: sqsgenerator.core.SqsResult

.. autoclass:: sqsgenerator.core.SqsResultInteractDouble
    :members:
    :undoc-members:
    :inherited-members:

.. autoclass:: sqsgenerator.core.SqsResultSplitDouble
    :members:
    :undoc-members:
    :inherited-members:


.. autoclass:: sqsgenerator.core.SqsConfiguration


.. autoclass:: sqsgenerator.core.SqsConfigurationDouble
    :members:
    :undoc-members:
    :inherited-members:
```
