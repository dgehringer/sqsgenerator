import os
print("pwd", "=", os.getcwd())
import pymatgen.core
import data
import numpy as np
from mpi4py import MPI

num_ranks = MPI.COMM_WORLD.Get_size()

cell = np.eye(3)*4.05

frac_coords = np.array([
    [0.0, 0.0, 0.0],
    [0.5, 0.5, 0.5]]
)

species = ["Fe", "Fe"]
s = pymatgen.core.Structure(cell, species, frac_coords)
s = s * (2, 2, 3)

S = data.Structure(s.lattice.matrix, s.frac_coords, ["Fe"]*12 + ["Ni"]*12, (True, True, True))

assert S.num_atoms == 24
assert S.pbc == (True, True, True)
assert np.allclose(S.frac_coords, s.frac_coords)
assert np.allclose(S.lattice, s.lattice.matrix)
assert np.allclose(S.distance_matrix, s.distance_matrix)

import sys
import iteration as it

shell_weights = {1: 1.0, 2: 0.5, 3: 0.3333333, 4: 0.25, 5: 0.2, 6: 1/6, 7: 1/7}
niterations = 1e7
settings = it.IterationSettings(S, np.ones((len(shell_weights),2,2)), np.ones((2,2)), shell_weights, int(niterations), 10, [8]*num_ranks, 1e-3, 1e-8, it.IterationMode.random)

print("IterationSettings.mode:", settings.mode)
print("IterationSettings.shell_weights:", settings.shell_weights, shell_weights == settings.shell_weights)
print("IterationSettings.num_species:", settings.num_species)
print("IterationSettings.num_shells:", settings.num_shells)
print("IterationSettings.num_atoms:", settings.num_atoms)
print("IterationSettings.num_iterations:", settings.num_iterations)
print("IterationSettings.parameter_weights:", settings.parameter_weights)
print("IterationSettings.target_objective", settings.target_objective)
print("IterationSettings.atol", settings.atol)
print("IterationSettings.rtol", settings.rtol)

if __name__ == "__main__":
    import time
    from mpi4py import MPI
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    head_rank = 0

    t0 = time.time()
    result, timings = it.pair_sqs_iteration(settings)
    t = time.time() - t0
    print(t/(niterations)*1e6, "microsec-per-permutation")
    if rank == head_rank:
        print("RANK:", rank)
        from pprint import pprint
        pprint(timings)
