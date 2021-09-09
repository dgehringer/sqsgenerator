
# How to use `sqsgenerator`

This is short tutorial, on how `sqsgenerator` works


## using the CLI interface

This section deals with the usage of the `sqsgenerator` package. A more granular documentation for the CLI can be found 
in the {ref}`CLI Reference <cli_reference>`. The CLI interface was built using the excellent [click](https://click.palletsprojects.com/en/8.0.x/) framework.

Once you have managed to {ref}`install <installation_guide>` `sqsgenerator` you should have a command `sqsgen` available
in your shell.

Make sure you can call the `sqsgen` command before starting off, using

```{code-block} bash
sqsgen --version
```

which should plot version information about `sqsgenerator` and also its dependencies.



````{note}

`sqsgenerator` uses a `dict`-like configuration, to store the parameters used for the iteration/analysis process. By
default the program assumes the configuration to be stored in a YAML file.

Therefore, most of `sqsgen`'s subcommands need such an input file to obtain the parameters.

By default the program searches for a file named **sqs.yaml** in the directory where it is executed
You can always specify a different one in case you want the program to execute a different one.

```{code-block} bash
sqsgen run iteration  # is equal to
sqsgen run iteration sqs.yaml
sqsgen run iteration path/to/my/custom/sqs_config.yaml
```

This applies to nearly all commands, except `sqsgen analyse` which by default expects **sqs.results.yaml**

````

We will go through the functionality of `sqsgenerator` using examples!
### Refactory metals - an ideal $\text{Re}_{0.5}\text{W}_{0.5}$