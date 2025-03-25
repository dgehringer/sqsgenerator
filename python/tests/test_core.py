import numpy as np
from sqsgenerator.core import single, parse_config, interact, systematic, optimize
from sqsgenerator import optimize


def test_systematic_all_results():
    config = dict(
        prec=single,
        mode=systematic,
        structure=dict(
            lattice=np.diag([1, 2, 3]),
            coords=np.array(
                [[0.0, 0.0, 0.0], [0.5, 0.5, 0.0], [0.0, 0.5, 0.5], [0.5, 0.0, 0.5]]
            ),
            species=[1] * 4,
            supercell=[2, 2, 1],
        ),
        pair_weights=0,
        composition={"sites": "H", "Ti": 8, "Al": 8},
    )
    results = optimize(config)
