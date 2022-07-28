import io
import sys
import math
import functools
import itertools
import numpy as np
from pymatgen.io.vasp import Poscar
from pymatgen.util.coord import pbc_shortest_vectors

def write_array(stream, array: np.ndarray, name=None, fmt="{0:.8f}"):
    stream.write("{}::array::begin\n".format(name))
    stream.write("{}::array::ndims {}\n".format(name, len(array.shape)))
    stream.write("{}::array::shape {}\n".format(name, ' '.join(map(str, array.shape))))
    stream.write("{}::array::data".format(name))
    for v in array.flat: stream.write(" " + fmt.format(v))
    stream.write("\n")
    stream.write("{}::array::end\n".format(name))


def nditer(A: np.ndarray):
    return itertools.product(*map(range, A.shape))

ABS_TOL = 1.0e-3
REL_TOL = 1.0e-8

isclose = functools.partial(math.isclose, rel_tol=REL_TOL, abs_tol=ABS_TOL)

if __name__ == "__main__":

    try:
        _, name, poscar_path = sys.argv
    except ValueError:
        print("I'm expecting exactly two arguments, namely a test case name and a path to a POSCAR file")
        sys.exit()
    structure = Poscar.from_file(poscar_path).structure
    l = structure.lattice.matrix
    d2 = structure.distance_matrix
    fc = structure.frac_coords
    vecs = pbc_shortest_vectors(structure.lattice, fc, fc)

    s = np.zeros_like(d2, dtype=int)
    heights = {0.0: []}

    shell_border_nearby = lambda x: any(isclose(x, h) for h in heights)
    closest_shell = lambda x: min(heights.keys(), key=lambda h : abs(x - h))

    for i, j in nditer(d2):
        d = d2[i ,j]
        if shell_border_nearby(d): heights[closest_shell(d)].append(d)
        else: heights[d] = [d]
        if j < i: continue

    shell_dists = [max(heights[k]) for k in sorted(heights.keys())]

    def find_shell(d):
        for i, (lower, upper) in enumerate(zip(shell_dists[:-1], shell_dists[1:])):
            if ((isclose(d, lower) or lower < d) and (isclose(d, upper) or upper > d)): return i + 1
        return len(shell_dists)

    for i, j in nditer(d2):
        d = d2[i ,j]
        actual_shell = find_shell(d)
        s[i,j] = actual_shell
        if i == j: s[i, j] = 0

    np.set_printoptions(2)

    numbers = np.array([site.specie.Z for site in structure])

    with open(f"{name}.test.data", "w") as stream:
        write_array(stream, l, "lattice")
        write_array(stream, numbers, "numbers", fmt="{0}")
        write_array(stream, fc, "fcoords")
        write_array(stream, d2, "distances")
        write_array(stream, s, "shells", fmt="{0}")
        write_array(stream, vecs, "vecs")

