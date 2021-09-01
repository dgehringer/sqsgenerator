
from attrdict import AttrDict

help = AttrDict(dict(
    analyse='Calculate SRO parameters of existing structures. Searches by default for a file named "sqs.result.yaml".',
    filename='A path to a file readable by sqsgenerator', # TODO: Add helplinks here
    input_format='The filetype of {filename}',
    pretty='Pretty print SQS results',
    dump_params='Dump input parameters - Append them to the newly generated output',
    output_format='Output format type',

))