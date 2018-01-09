
from libc.stdint cimport uint8_t, uint32_t, uint64_t

cdef extern from "<gmp.h>" nogil:
    ctypedef struct mpz_t

cdef extern from "include/utils.h" nogil:
    cdef void factorial_mpz(mpz_t mi_result, uint64_t n) nogil
    cdef inline uint32_t xor32() nogil
    cdef inline uint32_t xor64() nogil
    cdef inline uint32_t xor96() nogil
    cdef inline uint32_t xor128() nogil
    cdef inline uint32_t xor160() nogil
    cdef inline uint32_t xorwow() nogil
    cdef inline uint32_t rand_int(uint32_t n) nogil
    cdef void reseed_xor() nogil
    cdef bint knuth_fisher_yates_shuffle(uint8_t *configuration, size_t atoms) nogil

cdef extern from "include/rank.h" nogil:
    cdef void permutation_count_mpz(mpz_t mi_result, uint8_t *configuration, size_t atoms) nogil
    cdef uint64_t rank_permutation(uint8_t *configuration, size_t atoms, size_t species) nogil
    cdef void rank_permutation_mpz(mpz_t result, uint8_t *configuration, size_t atoms, size_t species) nogil
    cdef void unrank_permutation_mpz(uint8_t *configuration, size_t atoms, size_t *hist, size_t species, mpz_t mi_permutations, mpz_t mi_rank) nogil
    cdef void unrank_permutation(uint8_t *configuration, size_t atoms, size_t *hist, size_t species, uint64_t permutations, uint64_t rank) nogil
    cdef bint next_permutation_lex(uint8_t *configuration, size_t atoms) nogil
