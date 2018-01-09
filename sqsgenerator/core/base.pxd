from libc.stdint cimport uint8_t, uint32_t

cdef class BaseIterator:
    cdef readonly size_t atoms
    cdef readonly size_t shell_count
    cdef size_t species_count
    cdef int verbosity
    cdef readonly int seed

    cdef uint8_t[::1] configuration
    cdef size_t[::1] composition_hist
    cdef uint8_t[:, :] shell_number_matrix

    cdef double[:] mole_fractions_view
    cdef double[:] weights_view
    cdef double[:, :] distance_matrix
    cdef double[:, :] fractional_coordinates
    cdef double[:, :, :] distance_vectors

    cdef dict mole_fractions
    cdef dict weights
    cdef dict shell_distance_mapping
    cdef dict shell_neighbor_mapping
    cdef dict species_index_map

    cdef readonly object structure
    cdef readonly object lattice

    cdef uint8_t *shell_number_matrix_ptr
    cdef uint8_t *configuration_ptr
    cdef double *weights_ptr
    cdef double *mole_fractions_ptr
    cdef size_t *composition_hist_ptr

    cdef make_configuration(self, dict mole_fractions)
    cdef uint8_t[:] configuration_from_structure(self)
    cdef dict calculate_shell_neighbors(self, double[:, :] matrix)
    cdef substitute_distance_matrix(self, double[:, :] src, uint8_t[:, :] dest)
    cdef dict calculate_shells(self, double[:, :] matrix)