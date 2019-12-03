#from libc.stdint cimport uint8_t, uint32_t
import numpy as np
cimport numpy as np
from random import randint
from pymatgen import Structure
from pymatgen.util.coord import pbc_shortest_vectors
from collections import Counter
cimport sqsgenerator.core.utils as utils
from libc.math cimport fabs, fmax
cimport cython

cdef extern from "<stdlib.h>":
    cdef size_t RAND_MAX

cdef bint isclose(double a, double b, double rel_tol=1e-9, double abs_tol=0.0) nogil:
    """
     Checks for floating point numbers for equality

     Args:
         a (float): A floating point number to compare with another one
         b (float): The second number to compare

     Keyword Args:
         rel_tol (float): Relative tolerance
         abs_tol (float): Absolute tolerance

     Returns:
         bool: A boolean flag indicating equality
    """
    return fabs(a - b) <= fmax(rel_tol * fmax(fabs(a), fabs(b)), abs_tol)

cdef class BaseIterator:

    def __cinit__(self, structure, dict mole_fractions, dict weights, verbosity=0, **kwargs):
        self.structure = structure
        self.fractional_coordinates = structure.frac_coords
        self.lattice = structure.lattice
        self.atoms = len(self.structure.sites)
        vectors, d2 = pbc_shortest_vectors(self.lattice, self.fractional_coordinates, self.fractional_coordinates, return_d2=True)

        self.distance_matrix = np.sqrt(d2)
        self.distance_vectors = np.abs(vectors)
        self.mole_fractions = mole_fractions
        self.weights = weights

        self.shell_distance_mapping = self.calculate_shells(self.distance_matrix)

        self.configuration = np.ascontiguousarray(np.zeros((self.atoms,), dtype=np.uint8))
        self.shell_number_matrix = np.ascontiguousarray(np.zeros((self.atoms, self.atoms), dtype=np.uint8))

        self.substitute_distance_matrix(self.distance_matrix, self.shell_number_matrix)

        self.shell_neighbor_mapping = self.calculate_shell_neighbors(self.distance_matrix)

        self.shell_count = min(len(self.shell_neighbor_mapping), len(weights))

        self.species_count = len(mole_fractions)

        self.composition_hist = np.zeros((self.species_count), dtype=np.uintp)

        #Initializes self.mole_fractions_view
        self.make_configuration(mole_fractions)

        self.weights_view = np.ascontiguousarray(list(self.weights.values()))


        #self.sqs_constant_factor_matrix = self.sqs_make_constant_factor_matrix()
        #self.dosqs_constant_factor_matrix = self.dosqs_make_constant_factor_matrix()

        #self.print_verbose_information(verbosity=verbosity)
        self.seed = randint(1, RAND_MAX)
        self.verbosity = verbosity

        self.weights_ptr = <double*> &self.weights_view[0]
        self.mole_fractions_ptr = <double*> &self.mole_fractions_view[0]
        self.shell_number_matrix_ptr = <uint8_t*> &self.shell_number_matrix[0, 0]
        self.configuration_ptr = <uint8_t*> &self.configuration[0]
        self.composition_hist_ptr = <size_t*> &self.composition_hist[0]

    cdef make_configuration(self, dict mole_fractions):
        """
        Distributes the atoms according to the mole fractions. Corrects the mole fractions eventually if the mole
        fractions cannot be reached with integer numbers i.e 27 atoms with 0.5,0.5 composition

        Args:
            mole_fractions (dict): A dict specifying the final composition. It must have the following form::

                mole_fractions = {
                    'Al': 0.4,
                    'Ti': 0.35,
                    'B': 0.05
                }
        """
        cdef size_t global_cnt = 0
        cdef size_t atoms_per_species = 0;
        cdef size_t i = 0
        cdef size_t j = 0
        cdef size_t random_species = 0

        cdef list conf_list = []
        cdef mole_fraction_view_list = [0.0]*self.species_count
        cdef mole_fraction_item_list = list(mole_fractions.items())
        cdef dict corrected_mole_fractions = {}
        cdef list species_order = []
        self.species_index_map = {}

        # sort species in ascending order
        species_order = list(sorted(list(mole_fractions.keys())))
        for i, species in enumerate(species_order):
            mole_fraction = mole_fractions[species]
            atoms_per_species = round(self.atoms*mole_fraction)
            conf_list.extend([i]*atoms_per_species)
            corrected_mole_fractions[species] = float(atoms_per_species)
            self.species_index_map[species] = i
        while len(conf_list) < self.atoms:
            #Fill up with random atoms
            new_atom = utils.rand_int(self.species_count)
            species, mole_fraction = mole_fraction_item_list[new_atom]
            corrected_mole_fractions[species] = corrected_mole_fractions[species]+1.0
            conf_list.append(new_atom)

        for key in list(corrected_mole_fractions.keys()):
            self.composition_hist[self.species_index_map[key]] = corrected_mole_fractions[key]
            corrected_mole_fractions[key] /= float(self.atoms)

        for species, mole_fraction in mole_fractions.items():
            if not isclose(mole_fraction, corrected_mole_fractions[species]):
                self.mole_fractions = corrected_mole_fractions
                print('Mole fractions were corrected to: {0}'.format(corrected_mole_fractions))
                break

        for species, mole_fraction in self.mole_fractions.items():
            mole_fraction_view_list[self.species_index_map[species]] = mole_fraction

        self.mole_fractions_view = np.ascontiguousarray(mole_fraction_view_list)

        self.configuration = np.ascontiguousarray(conf_list, dtype=np.uint8)


    cdef dict calculate_shells(self, double[:, :] matrix):
        """
        Determines how many shells are available are available, by counting distinct distance values

        Args:
            matrix (:class:`numpy.ndarray`): The distance matrix of shape ``(atoms,atoms)``, where the value :math:`a_{ij}`
                represents the distance between the i-th and the j-th atom.

        Note:
            The shell index starts with 1 not with 0

        Returns:
            dict: A dictionary where the key indicates the shell number , and the value the corresponding shell radius.
                The dictionary has the following form::

                    shell_dict = {
                        1: 2.943,
                        2: 4.563,
                        3: 6.873,
                        4: 7.345
                    }
        """
        cdef double[:] atoms = matrix[0]
        cdef double current_number, shell
        cdef size_t i = 0
        shells = []

        for i in range(1, self.atoms):
            current_number = atoms[i]
            is_class = False
            for shell in shells:
                if isclose(shell, current_number, rel_tol=1e-4):
                    is_class = True
                    break
            if not is_class:
                shells.append(current_number)
        shells = sorted(shells)
        shell_dict = {}
        for i, shell_distance in enumerate(shells):
            shell_dict[i + 1] = shell_distance
        return shell_dict

    @cython.boundscheck(False)
    @cython.wraparound(False)
    cdef substitute_distance_matrix(self, double[:, :] src, uint8_t[:, :] dest):
        """
         Internal utility method for replacing the distance entries in ``matrix`` by the shell number the entry can be mapped
         to.

         Args:
             shell_dict (dict): A dictionary containing the shell radius. For the form of the dictionary see
                 :func:`core.calculate_shells`
             matrix (:class:`numpy.ndarray`): The distance matrix, which corresponds to ``shell_dict``
         """
        cdef Py_ssize_t i = 0, j = 0
        cdef int shell
        cdef double distance
        for shell, distance in self.shell_distance_mapping.items():
            for i in range(self.atoms):
                for j in range(i, self.atoms):
                    if isclose(src[i,j], distance, rel_tol=1e-4):
                        dest[i, j] = shell
                        dest[j, i] = shell


    cdef dict calculate_shell_neighbors(self, double[:, :] matrix):
        """
         Determines the amount of next neighbors (:math:`M_j`) in each shell. By counting distinct values in the distance matrix

         Args:
             shell_dict (dict): A dictionary containing the shell radius. For the form of the dictionary see
                 :func:`core.calculate_shells`
             matrix (:class:`numpy.ndarray`): The distance matrix, which corresponds to ``shell_dict``

         Returns:
             dict: A dictionary with shell numbers as keys and the corresponding number of atoms in this shell.
                 The dictionary is structured in the following fashion::

                     next_neighbors = {
                         1 : 12
                         2: 8
                         3: 6
                         4: 10
                     }
         """
        neighbor_dict = {}
        neighbor_dict.update(Counter(np.asarray(self.shell_number_matrix[0])))
        del neighbor_dict[0.0]
        return neighbor_dict

    cdef uint8_t[:] configuration_from_structure(self):
        cdef size_t species_count = 0
        cdef uint8_t[:] configuration = np.ascontiguousarray(np.zeros((self.atoms), dtype=np.uint8))
        cdef Py_ssize_t i = 0

        for i, site in enumerate(self.structure.sites):
            configuration[i] = self.species_index_map[site.specie.name]

        return configuration

    def configuration_to_structure(self, uint8_t[:] configuration):
        index_species_map = {v: k for k, v in self.species_index_map.items()}
        species_list = [index_species_map[configuration[i]] for i in range(self.atoms) if index_species_map[configuration[i]] != '0']
        coord_list = [self.fractional_coordinates[i] for i in range(self.atoms) if index_species_map[configuration[i]] != '0']
        structure = Structure(self.structure.lattice, species_list, coord_list)
        return structure