import pprint

import attrdict

from sqsgenerator import sqs_optimize


if __name__ == '__main__':
    configuration = dict(
        structure=dict(file='docs/source/examples/b2.vasp',
                       supercell=(2,2,2)),
        composition=dict(W=8, Re=8),
        shell_weights={1: 1.0},
        iterations=1e7
    )

    sqs_optimize(configuration)