import json

import numpy as np
from pymatgen.core import Structure, Lattice
from scipy.spatial import distance_matrix

NUM_ATOMS = 32

FRAC_COORDS = np.random.uniform(size=(NUM_ATOMS, 3))
LATTICE = np.random.uniform(size=(3, 3)) * 4


def shell_matrix(s: Structure):
    d = s.distance_matrix
    distances = sorted(np.unique(d).tolist())
    sm = -np.ones_like(d, dtype=np.int_)
    get_shell = distances.index
    for i in range(d.shape[0]):
        for j in range(i, d.shape[1]):
            sm[i, j] = sm[j, i] = get_shell(d[i, j])

    assert np.all(sm >= 0)
    return sm


def dump_structure(s: Structure):
    with open("test.json", "w") as handle:
        print(
            json.dumps(
                dict(
                    lattice=s.lattice.matrix.tolist(),
                    frac_coords=s.frac_coords.tolist(),
                    distance_matrix=s.distance_matrix.tolist(),
                    shell_matrix=shell_matrix(s).tolist()
                )
            )
        , file=handle)


dump_structure(Structure(Lattice(LATTICE), ["Al"] * NUM_ATOMS, FRAC_COORDS))