"""
A data file storing the CLI's help strings
"""

from attrdict import AttrDict
from sqsgenerator.io import default_adapter


command_help = AttrDict(dict(
    analyse='Calculate SRO parameters of existing structures. Searches by default for a file named "sqs.result.yaml".',
    compute=dict(
        command='Compute quantities to determine how computationally demanding you input file is',
        rank='Calculate a unique number identifying the structure in the input file',
        shell_distances='Calculate the default radii of the coordination shells',
        total_permutations='The number of iterations carried out. Useful for for "systematic" mode',
        estimated_time='Estimate the runtime of your current settings'
    ),
    export='Export sqsgenerator\'s results into structure files',
    params=dict(
        command='Utilities to check the parameters that are going to be used in the SQS iteration',
        show='Display all or certain input parameters',
        check='Check if your input settings will work'
    ),
    iteration='Perform a SQS iteration'
))

parameter_help = AttrDict(dict(
    filename='A path to a file readable by sqsgenerator',  # TODO: Add help links here
    input_format='The filetype of {filename}',
    pretty='Pretty print SQS results',
    dump_params='Dump input parameters - Append them to the newly generated output',
    output_format='Output format type',
    format='Output format for the structure file(s). The format must be supported by the specified backend',
    writer=f'Backend for writing the structure files. Default is "{default_adapter()}"',
    compress='Gather the output structure files in an compressed archive',
    output_file='A file name prefix. The file extension is chosen automatically',
    param='Display only this input parameter(s)',
    inplace='Dump the computed input parameters in the input settings file',
    dump_format='Format used to store sqsgenerator\'s results',
    dump_include='Include field in sqsgenerator\'s results',
    dump='Dump sqsgenerator\'s results in a file',
    log_level='log level of the core extension. Should be "trace" used for error reporting',
    export='Export the results obtained from the SQS iteration as structure files',
    minimal='Include only configurations with minimum objective function in the results',
    similar='If the minimum objective is degenerate include also ranks with same parameters but different rank'
))
