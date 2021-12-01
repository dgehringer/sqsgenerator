import pprint
from operator import itemgetter as item
from sqsgenerator import sqs_optimize, export_structures


if __name__ == '__main__':
    configuration = dict(
        structure=dict(file='docs/source/examples/b2.vasp',
                       supercell=(2,2,2)),
        composition=dict(W=8, Re=8),
        shell_weights={1: 1.0},
        iterations=1e10
    )

    result, timings = sqs_optimize(configuration, make_structures=True)
    print(result)
    export_structures(result, writer='pymatgen', functor=item('structure'))