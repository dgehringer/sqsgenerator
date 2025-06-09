import collections
import json
import math
import sys
import tempfile
from typing import NamedTuple, TypeVar

import numpy as np
import pytest

from sqsgenerator import optimize
from sqsgenerator.core import (
    Prec,
    SqsConfiguration,
    SqsResult,
    Structure,
    double,
    load_result_pack,
    parse_config,
    random,
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
        threads_per_rank=1,
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
        ranks.discard(int(sol.rank()))

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


@pytest.mark.parametrize("prec", [single, double])
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


@pytest.mark.skipif(
    sys.platform.startswith("win"), reason="Test not supported on Windows"
)
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
                assert np.allclose(sa.frac_coords, sb.frac_coords)


K = TypeVar("K")
V = TypeVar("V")


def inverse(d: dict[K, V]) -> dict[V, K]:
    return {v: k for k, v in d.items()}


class OptimizationHelper(NamedTuple):
    structure: Structure
    shell_matrix: np.ndarray
    shell_weights: dict[int, float]
    pair_list: list[tuple[int, int, int]]
    shell_map: dict[int, int]
    species_map: dict[int, int]
    shell_hist: collections.Counter[int]
    sro_shape: tuple[int, int, int]

    @classmethod
    def from_structure(
        cls,
        structure: Structure,
        shell_radii: list[float],
        shell_weights: dict[int, float],
    ):
        return cls(
            structure,
            sm := structure.shell_matrix(shell_radii, 1.0e-3),
            w := shell_weights,
            pair_list := [
                (i, j, int(s))
                for (i, j), s in np.ndenumerate(sm)
                if i > j
                if int(s) in w
            ],
            shell_map := {s: i for i, s in enumerate(w)},
            species_map := {v: i for i, v in enumerate(sorted(set(structure.species)))},
            collections.Counter(s for *_, s in pair_list),
            (len(shell_map), nspecies := len(species_map), nspecies),
        )


def assert_bonds_correct_interact(results: SqsResult, config: SqsConfiguration) -> None:
    helper: OptimizationHelper | None = None

    for _, solutions in results:
        first, *rest = solutions
        helper = helper or OptimizationHelper.from_structure(
            first.structure(), config.shell_radii[0], config.shell_weights[0]
        )
        sros = np.zeros(shape=helper.sro_shape)
        species = first.species
        nspecies = len(helper.species_map)
        for i, j, s in helper.pair_list:
            sros[
                helper.shell_map[s],
                helper.species_map[species[i]],
                helper.species_map[species[j]],
            ] += 1
        for i in range(nspecies):
            for j in range(i + 1, nspecies):
                sum_ = sros[:, i, j] + sros[:, j, i]
                sros[:, j, i] = sum_
                sros[:, i, j] = sum_
        for i, (bonds, sro_per_shell) in enumerate(zip(sros, first.sro())):
            nbonds = sum(
                bonds[si, sj] for si in range(nspecies) for sj in range(si, nspecies)
            )
            assert helper.shell_hist[inverse(helper.shell_map)[i]] == nbonds
            assert np.allclose(bonds, 1 - sro_per_shell)


@pytest.mark.parametrize("prec", [single, double])
@pytest.mark.parametrize(
    "iteration_mode",
    [systematic, random],
)
def test_bonds_correct_one_sublattice(prec, iteration_mode):
    settings = default_settings(prec)
    del settings["pair_weights"]

    settings["prefactors"] = 1
    settings["iteration_mode"] = iteration_mode
    settings["iterations"] = 15000

    config = parse_config(settings)
    results = optimize(config)
    assert_bonds_correct_interact(results, config)


@pytest.mark.parametrize("prec", [single, double])
def test_bonds_correct_one_sublattice_non_exhaustive(prec):
    settings = default_settings(prec)
    del settings["pair_weights"]

    settings["prefactors"] = 1
    settings["iteration_mode"] = random
    settings["iterations"] = 1200
    settings["structure"]["species"] = [1, 1, 2, 2]
    settings["structure"]["supercell"] = [2, 2, 2]
    settings["composition"] = {"sites": "He", "Ti": 6, "Al": 5, "O": 5}

    config = parse_config(settings)
    results = optimize(config)
    assert_bonds_correct_interact(results, config)
