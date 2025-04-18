import collections
import math
import tempfile

import numpy as np
import pytest

from sqsgenerator import optimize
from sqsgenerator.core import (
    Prec,
    double,
    load_result_pack,
    parse_config,
    single,
    systematic,
)


def default_settings(prec: Prec):
    return dict(
        prec=prec,
        iteration_mode=systematic,
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
    config_structure = config.structure()
    site_by_index = {index: as_tuple(coords) for index, coords, _ in config_structure}
    for sol in solutions:
        structure = sol.structure()
        assert math.isclose(sol.objective, 0.0)
        assert collections.Counter(structure.species) == collections.Counter(
            {22: 8, 13: 8}
        )
        ranks.discard(int(sol.rank(10)))

        for index, coords, _ in structure:
            assert as_tuple(coords) == site_by_index[index]
    assert not ranks


def as_tuple(coords: np.ndarray[float], prec: int = 5) -> tuple[float, float, float]:
    return (
        round(float(coords[0]), prec),
        round(float(coords[1]), prec),
        round(float(coords[2]), prec),
    )


def coords(site) -> tuple[float, float, float]:
    return as_tuple(site.frac_coords)


@pytest.mark.parametrize("prec", [single])
def test_systematic_all_results_partial(prec):
    settings = default_settings(prec)
    settings["structure"] = dict(
        lattice=np.diag([1, 2, 3]),
        coords=np.array(
            [[0.0, 0.0, 0.0], [0.5, 0.5, 0.0], [0.0, 0.5, 0.5], [0.5, 0.0, 0.5]]
        ),
        species=[1, 2] * 2,
        supercell=[2, 2, 2],
    )

    config = parse_config(settings)
    results = optimize(config)
    assert results.num_results() == config.iterations
    solutions = [sol for _, rset in results for sol in rset]
    # find all the sites that are occupied by a hydrogen atom
    config_structure = config.structure()
    config_sites = set(map(as_tuple, config_structure.frac_coords))

    assert len(config_sites) == len(config_structure)

    random_sites = {index for index, _, specie in config_structure if specie == 1}
    assert len(random_sites) == 16

    site_by_index = {index: as_tuple(coords) for index, coords, _ in config_structure}

    for sol in solutions:
        structure = sol.structure()
        assert math.isclose(sol.objective, 0.0)
        assert len(structure) == len(config_structure)
        # check that the number of species is correct for each resulting structure
        assert collections.Counter(structure.species) == collections.Counter(
            {22: 8, 13: 8, 2: 16}
        )
        assert set(map(as_tuple, structure.frac_coords)) == config_sites
        # ensure that the sites have not been reordered

        # ensure that the atoms are on the right sublattice
        assert {
            index
            for index, specie in enumerate(structure.species)
            if specie in {22, 13}
        } == random_sites

        for index, coords, _ in structure:
            assert as_tuple(coords) == site_by_index[index]


@pytest.mark.parametrize("prec", [single, double])
def test_structure_pack_io(prec):
    settings = default_settings(prec)

    config = parse_config(settings)
    results = optimize(config)

    assert type(results) is type(load_result_pack(results.bytes(), prec))

    with tempfile.NamedTemporaryFile(mode="wb") as f:
        f.write(results.bytes())
        f.flush()
        loaded = load_result_pack(f.name, prec)
        assert type(results) is type(loaded)
        for a, b in zip(loaded, results):
            (aobj, ares), (bobj, bres) = a, b
            assert math.isclose(aobj, bobj)
            assert len(ares) == len(bres)
            for ar, br in zip(ares, bres):
                assert math.isclose(ar.objective, br.objective)
                assert np.allclose(ar.sro(), br.sro())
                sa, sb = ar.structure(), br.structure()
                # TODO: investrigate
                # sa == sb
                assert np.allclose(sa.frac_coords, sb.frac_coords)
