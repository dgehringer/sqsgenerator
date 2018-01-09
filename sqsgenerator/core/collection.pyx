
cdef class ConfigurationCollection:

    def __cinit__(self, size_t max_size, size_t atoms, size_t shell_count, size_t species_count, size_t dimension=1):
        self._inner = conf_collection_init(max_size, atoms, shell_count * species_count * species_count * dimension)

    cdef bint add(self, double objective, uint8_t *configuration, double *decomposition) nogil:
        return conf_collection_add(self._inner, objective, configuration, decomposition)

    cdef double *get_decomposition(self, size_t index) nogil:
        return conf_collection_get_decomp(self._inner, index)

    cdef double get_objective(self, size_t index) nogil:
        return conf_collection_get_objective(self._inner, index)

    cdef uint8_t *get_configuration(self, size_t index) nogil:
        return conf_collection_get_conf(self._inner, index)

    cdef size_t size(self) nogil:
        return self._inner.size

    cdef double best_objective(self) nogil:
        return self._inner.best_objective

    def __dealloc__(self):
        conf_collection_destroy(self._inner)