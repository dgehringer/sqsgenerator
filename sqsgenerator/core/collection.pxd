
from libc.stdint cimport uint8_t, uint32_t, uint64_t


cdef extern from "include/conf_collection.h" nogil:
    ctypedef struct conf_collection_t:
        size_t size
        double best_objective

    cdef conf_collection_t* conf_collection_init(size_t max_size, size_t atoms, size_t decomp_size) nogil
    cdef uint8_t* conf_collection_get_conf(conf_collection_t* c, size_t index) nogil
    cdef double* conf_collection_get_decomp(conf_collection_t* c, size_t index) nogil
    cdef double conf_collection_get_objective(conf_collection_t* c, size_t index) nogil
    cdef bint conf_collection_add(conf_collection_t* c, double objective, uint8_t *conf, double* decomp) nogil
    cdef void conf_collection_destroy(conf_collection_t* c) nogil

cdef class ConfigurationCollection:

    cdef conf_collection_t * _inner;

    cdef bint add(self, double objective, uint8_t *configuration, double *decomposition) nogil
    cdef double *get_decomposition(self, size_t index) nogil
    cdef double get_objective(self, size_t index) nogil
    cdef uint8_t *get_configuration(self, size_t index) nogil
    cdef size_t size(self) nogil
    cdef double best_objective(self) nogil