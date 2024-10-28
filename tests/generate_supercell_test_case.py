import json
import random
import numpy as np
from pymatgen.core import Structure, Lattice
from pymatgen.core.periodic_table import Element, Specie

NUM_ATOMS = 8
SHAPE = (3, 2, 1)
SPECIES = [Element.from_Z(random.randint(1, 100)) for _ in range(NUM_ATOMS)]
FRAC_COORDS = np.random.uniform(size=(NUM_ATOMS, 3))
LATTICE = np.random.uniform(size=(3, 3)) * 4
STRUCTURE = Structure(LATTICE, SPECIES, FRAC_COORDS)



def dump_structure(s: Structure):
    return dict(
            lattice=s.lattice.matrix.tolist(),
            frac_coords=s.frac_coords.tolist(),
            species=[site.specie.number for site in s],
            pbc=[True, True, True],
        )


with open("case1.supercell.json", "w") as handle:
    print(
        json.dumps(dict(
            structure=dump_structure(STRUCTURE),
            supercell=dump_structure(STRUCTURE * SHAPE),
            shape=list(SHAPE),
        )),
        file=handle
    )
