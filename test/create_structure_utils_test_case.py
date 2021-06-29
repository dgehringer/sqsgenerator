import io
import sys
import numpy as np
from pymatgen.io.vasp import Poscar
from pymatgen.util.coord import pbc_shortest_vectors

def write_array(stream, array: np.ndarray, name=None, fmt="{0:.8f}"):
    stream.write(f"{name}::array::begin\n")
    stream.write(f"{name}::array::ndims {len(array.shape)}\n")
    stream.write(f"{name}::array::shape {' '.join(map(str, array.shape))}\n")
    stream.write(f"{name}::array::data")
    for v in array.flat: stream.write(" " + fmt.format(v))
    stream.write("\n")
    stream.write(f"{name}::array::end\n")

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

    shell_prec = 5
    rounded = np.round(d2, shell_prec)
    indices = np.unique(rounded).tolist()
    get_index = np.vectorize(lambda x: indices.index(x))

    s = get_index(rounded)

    with open(f"{name}.test.data", "w") as stream:
        write_array(stream, l, "lattice")
        write_array(stream, fc, "fcoords")
        write_array(stream, d2, "distances")
        write_array(stream, s, "shells", fmt="{0}")
        write_array(stream, vecs, "vecs")

