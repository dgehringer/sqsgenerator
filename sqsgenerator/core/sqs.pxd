from libc.stdint cimport uint8_t, uint32_t, uint64_t
cimport sqsgenerator.core.base

cdef class SqsIterator(base.BaseIterator):

    cdef double[:, :] constant_factor_matrix
    cdef double *constant_factor_matrix_ptr

    cdef double[:, :] make_constant_factor_matrix(self)
    cdef alpha_to_dict(self, double[:, :, :]  alpha_decomposition)
    cdef double calculate_parameter(self, uint8_t* configuration, double *constant_factor_matrix, double* alpha_decomposition) nogil
    cdef void reset_alpha_results(self, double* alpha_decomposition) nogil