import collections
import json
import math

import pytest
import numpy as np
from sqsgenerator.core import (
    single,
    double,
    parse_config,
    interact,
    systematic,
    optimize,
    Prec,
)
from sqsgenerator import optimize


def default_settings(prec: Prec):
    return dict(
        prec=prec,
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


@pytest.mark.parametrize("prec", [single, double])
def test_systematic_all_results(prec):
    settings = default_settings(prec)

    config = parse_config(settings)
    results = optimize(config)
    assert config.iterations == 12870
    assert results.num_results() == config.iterations
    solutions = [sol for _, rset in results for sol in rset]
    ranks = set(range(1, config.iterations + 1))
    for sol in solutions:
        assert math.isclose(sol.objective, 0.0)
        assert collections.Counter(sol.structure().species) == collections.Counter(
            {22: 8, 13: 8}
        )
        ranks.discard(int(sol.rank(10)))
    assert not ranks


@pytest.mark.parametrize("prec", [single, double])
def test_systematic_all_results_partial(prec):
    return
    settings = dict(
        prec=prec,
        mode=systematic,
        structure=dict(
            lattice=np.diag([1, 2, 3]),
            coords=np.array(
                [[0.0, 0.0, 0.0], [0.5, 0.5, 0.0], [0.0, 0.5, 0.5], [0.5, 0.0, 0.5]]
            ),
            species=[1, 2] * 2,
            supercell=[2, 2, 2],
        ),
        pair_weights=0,
        composition={"sites": "H", "Ti": 8, "Al": 8},
    )

    config = parse_config(settings)
    results = optimize(config)
    assert results.num_results() == config.iterations
    solutions = [sol for _, rset in results for sol in rset]
    ranks = set(range(1, config.iterations + 1))
    for sol in solutions:
        assert math.isclose(sol.objective, 0.0)
        assert collections.Counter(sol.structure().species) == collections.Counter(
            {22: 8, 13: 8, 2: 16}
        )
        ranks.discard(int(sol.rank(10)))
    assert not ranks
