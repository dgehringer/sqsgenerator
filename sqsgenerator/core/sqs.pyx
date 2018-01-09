from libc.stdint cimport uint8_t, uint32_t, uint64_t
from libc.string cimport memset
from libc.stdlib cimport malloc, free
from libc.math cimport fabs
from sqsgenerator.core.utils cimport next_permutation_lex, knuth_fisher_yates_shuffle, reseed_xor, unrank_permutation
from sqsgenerator.core.collection cimport ConfigurationCollection
cimport numpy as np
cimport cython
cimport base
cimport openmp
from collections import Counter
from cython.parallel import parallel
from math import factorial
import time
import multiprocessing
import numpy as np

cdef extern from '<float.h>':
    cdef double DBL_MAX

ctypedef bint (*next_permutation_function) (uint8_t *configuration, size_t atoms) nogil

cdef class SqsIterator(base.BaseIterator):

    #cdef double[:, :] constant_factor_matrix
    #cdef double *constant_factor_matrix_ptr

    def __cinit__(self, structure, dict mole_fractions, dict weights, verbosity=0):
        #super(SqsIterator, self).__cinit__(structure, mole_fractions, weights, verbosity=verbosity)
        self.constant_factor_matrix = self.make_constant_factor_matrix()
        self.constant_factor_matrix_ptr = <double*> &self.constant_factor_matrix[0, 0]

    cdef double[:, :] make_constant_factor_matrix(self):
        cdef double[:, :] constant_factor_matrix = np.ascontiguousarray(np.zeros((self.atoms, self.atoms)))

        cdef Py_ssize_t i = 0, j = 0
        cdef double weight
        cdef double value
        cdef uint8_t shell

        for i in range(self.atoms):
            for j in range(i, self.atoms):
                if i == j:
                    constant_factor_matrix[i, j] = 0.0
                    continue
                if i != j:
                    shell = self.shell_number_matrix[i,j]
                    weight = self.weights[shell] if shell in self.weights else 0.0
                    value = weight / (2* self.shell_neighbor_mapping[shell] * self.atoms) if shell in self.weights else 0.0

                    constant_factor_matrix[i, j] = value
                    constant_factor_matrix[j, i] = value

        return constant_factor_matrix

    @cython.boundscheck(False)
    @cython.wraparound(False)
    @cython.cdivision(True)
    cdef double calculate_parameter(self, uint8_t* configuration, double *constant_factor_matrix, double* alpha_decomposition) nogil:
        cdef size_t i = 0, j = 0, k = 0
        cdef uint8_t current_species
        cdef uint8_t compare_species
        cdef uint8_t shell
        cdef double alpha = 0,
        cdef double current_bond_ratio = 0.0
        cdef double current_alpha

        cdef int d1 = self.species_count * self.species_count, d2 = self.species_count

        for i in range(self.atoms):
            compare_species = configuration[i]
            for j in range(i, self.atoms):
                current_species = configuration[j]
                shell = self.shell_number_matrix_ptr[i * self.atoms + j] - 1
                if current_species != compare_species and -1 < shell <= self.shell_count - 1:
                    current_bond_ratio = alpha_decomposition[shell * d1 + compare_species * d2 + current_species]
                    current_bond_ratio += constant_factor_matrix[i * self.atoms + j] / (
                            self.mole_fractions_ptr[current_species] * self.mole_fractions_ptr[compare_species])
                    alpha_decomposition[shell * d1 + compare_species * d2 + current_species] = current_bond_ratio
                    alpha_decomposition[shell * d1 + current_species * d2 + compare_species] = current_bond_ratio

        for i in range(self.shell_count):
            for j in range(self.species_count):
                for k in range(self.species_count):
                    if j != k:
                        current_bond_ratio = alpha_decomposition[i * d1 + j * d2 + k]
                        current_alpha = self.weights_ptr[i] / 2 - current_bond_ratio
                        alpha_decomposition[i * d1 + j * d2 + k] = current_alpha
                        alpha += current_alpha

        return alpha

    def iteration(self, iterations=100000, output_structures=10, objective=0.0):
        #Definition
        cdef double best_alpha
        cdef double alpha
        cdef double objective_value
        cdef double *alpha_decomposition_ptr
        cdef bint all_output_structures_flag = output_structures == 'all'
        cdef ConfigurationCollection collection

        if objective == float('inf'):
            objective_value = DBL_MAX
        elif objective == float('-inf'):
            objective_value = -DBL_MAX
        else:
            objective_value = objective

        cdef double[:, :, :] alpha_decomposition = np.ascontiguousarray(np.zeros((self.shell_count, self.species_count, self.species_count)))
        alpha_decomposition_ptr = <double*> &alpha_decomposition[0, 0, 0]

        cdef size_t i = 0, j = 0, k = 0

        shared_collection = ConfigurationCollection(output_structures if not all_output_structures_flag else 0, self.atoms, self.shell_count, self.species_count)

        self.reset_alpha_results(alpha_decomposition_ptr)
        best_alpha = 1e15

        if iterations == 'all':
            self.sort_numpy(self.configuration)
            composition = dict(Counter(np.asarray(self.configuration)))

            total_iterations = factorial(self.atoms)
            current_iteration = 0
            for species, amount in composition.items():
                total_iterations = total_iterations/factorial(amount)
            print('Configurations to check: {0}'.format(total_iterations))

            t0 = time.time()
            while next_permutation_lex(self.configuration_ptr, self.atoms):
                current_iteration += 1
                alpha = self.calculate_parameter(self.configuration_ptr, self.constant_factor_matrix_ptr, alpha_decomposition_ptr)

                if objective_value == -DBL_MAX:
                    pass
                elif objective_value == DBL_MAX:
                    alpha = -alpha
                else:
                    alpha = fabs(alpha-objective_value)

                if alpha <= shared_collection.best_objective():
                    shared_collection.add(alpha, self.configuration_ptr, alpha_decomposition_ptr)
                    reseed_xor()
                self.reset_alpha_results(alpha_decomposition_ptr)
        else:
            t0 = time.time()
            for i in range(iterations):
                knuth_fisher_yates_shuffle(self.configuration_ptr, self.atoms)
                alpha = self.calculate_parameter(self.configuration_ptr, self.constant_factor_matrix_ptr, alpha_decomposition_ptr)

                if objective_value == -DBL_MAX:
                    pass
                elif objective_value == DBL_MAX:
                    alpha = -alpha
                else:
                    alpha = fabs(alpha-objective_value)

                if alpha <= shared_collection.best_objective():
                    shared_collection.add(alpha, self.configuration_ptr, alpha_decomposition_ptr)
                    reseed_xor()
                self.reset_alpha_results(alpha_decomposition_ptr)

        total = time.time() - t0

        self.reset_alpha_results(alpha_decomposition_ptr)
        #for i in range(shared_collection.size()):
            #assert isclose(shared_collection.best_objective, fabs(self.calculate_parameter_ptr(conf_collection_get_conf(shared_collection, i), self.sqs_constant_factor_matrix, &alpha_decomposition[0,0,0])))
            #self.reset_alpha_results(alpha_decomposition)

        structure_list = [self.configuration_to_structure(<uint8_t[:self.atoms]>shared_collection.get_configuration(i)) for i in range(shared_collection.size())]
        decomp_list = [self.alpha_to_dict(np.asarray(<double[:self.shell_count, :self.species_count, :self.species_count]>shared_collection.get_decomposition(i))) for i in range(shared_collection.size())]

        lps = total_iterations if iterations == 'all' else iterations

        return structure_list, decomp_list, lps, total/lps

    cdef alpha_to_dict(self, double[:, :, :]  alpha_decomposition):
        rearranged_alphas = {}
        cdef size_t i = 0, j = 0, k = 0

        species = list(self.mole_fractions.keys())
        for i in range(self.species_count):
            for j in range(i, self.species_count):
                if i != j:
                    alphas = []
                    for k in range(self.shell_count):
                        alphas.append(alpha_decomposition[k, i, j] * 2)
                    rearranged_alphas['{0}-{1}'.format(species[i], species[j])] = alphas

        return rearranged_alphas

    @cython.boundscheck(False)
    @cython.wraparound(False)
    cdef void reset_alpha_results(self, double* alpha_decomposition) nogil:
        memset(alpha_decomposition, 0, sizeof(double) * self.shell_count * self.species_count * self.species_count)

    def calculate_alpha(self):
        cdef int species_count = len(
            set(
                list(
                    map(lambda site: site.specie.name, self.structure.sites)
                )
            )
        )
        cdef size_t old_species_count = self.species_count
        self.species_count = species_count
        cdef double alpha
        cdef double[:, :, :] alpha_decomposition = np.ascontiguousarray(np.zeros((self.shell_count, self.species_count, self.species_count)))
        cdef double *alpha_decomposition_ptr = <double*> &alpha_decomposition[0, 0, 0]
        cdef uint8_t[:] configuration = self.configuration_from_structure()
        cdef uint8_t *configuraton_ptr = <uint8_t*> &configuration[0]

        self.reset_alpha_results(alpha_decomposition_ptr)
        alpha = self.calculate_parameter(configuraton_ptr, self.constant_factor_matrix_ptr, alpha_decomposition_ptr)

        rearranged_alphas = self.alpha_to_dict(alpha_decomposition)

        self.species_count = old_species_count
        return rearranged_alphas

    def sort_numpy(self, uint8_t[:] a, kind='quick'):
        np.asarray(a).sort(kind=kind)

cdef class ParallelSqsIterator(SqsIterator):

    cdef size_t num_threads

    def __cinit__(self, structure, dict mole_fractions, dict weights, verbosity=0, num_threads=multiprocessing.cpu_count()):
        self.num_threads = num_threads

    @cython.boundscheck(False)
    def iteration(self, iterations=100000, output_structures=10, objective=0.0, int threads=multiprocessing.cpu_count()):
        #Definition
        cdef int thread_id
        cdef bint all_flag = iterations == 'all'
        cdef bint all_output_structures_flag = output_structures == 'all'
        cdef uint64_t permutations
        cdef uint64_t start_permutation
        cdef uint64_t end_permutation
        cdef uint64_t c_iterations
        cdef uint8_t* local_configuration
        cdef size_t i = 0, j = 0, k = 0
        cdef double local_alpha
        cdef double objective_value = DBL_MAX if objective == float('inf') else (-DBL_MAX if objective == float('-inf') else objective)
        cdef double* local_alpha_decomposition
        cdef ConfigurationCollection collection
        cdef next_permutation_function next_permutation_function_ptr

        shared_collection = ConfigurationCollection(output_structures if not all_output_structures_flag else 0, self.atoms, self.shell_count, self.species_count)

        openmp.omp_set_num_threads(self.num_threads)

        if iterations == 'all':
            self.sort_numpy(self.configuration)
            composition = dict(Counter(np.asarray(self.configuration)))
            total_iterations = factorial(self.atoms)
            current_iteration = 0
            for species, amount in composition.items():
                total_iterations = total_iterations/factorial(amount)
            print('Configurations to check: {0}'.format(total_iterations))
            permutations = total_iterations
        else:
            c_iterations = iterations
        t0 = time.time()
        with nogil, parallel():

            thread_id = openmp.omp_get_thread_num()
            local_configuration = <uint8_t*>malloc(sizeof(uint8_t)*self.atoms)
            local_alpha_decomposition = <double*>malloc(sizeof(double)*self.shell_count*self.species_count*self.species_count)

            for i in range(self.atoms):
                local_configuration[i] = self.configuration[i]
            self.reset_alpha_results(local_alpha_decomposition)

            start_permutation = thread_id*(permutations if all_flag else c_iterations)/self.num_threads
            end_permutation = (thread_id+1)*(permutations if all_flag else c_iterations)/self.num_threads+1

            if thread_id+1 == self.num_threads:
                end_permutation = permutations if all_flag else c_iterations
            if thread_id == 0:
                start_permutation = 1

            if all_flag:
                next_permutation_function_ptr = next_permutation_lex
            else:
                next_permutation_function_ptr = knuth_fisher_yates_shuffle

            if all_flag:
                unrank_permutation(local_configuration, self.atoms, self.composition_hist_ptr, self.species_count, permutations, start_permutation)
            else:
                knuth_fisher_yates_shuffle(local_configuration, self.atoms)

            for j in range(start_permutation, end_permutation):
                local_alpha = self.calculate_parameter(local_configuration, self.constant_factor_matrix_ptr, local_alpha_decomposition)

                if objective_value == -DBL_MAX:
                    pass
                elif objective_value == DBL_MAX:
                    local_alpha = -local_alpha
                else:
                    local_alpha = fabs(local_alpha-objective_value)

                if local_alpha <= shared_collection.best_objective():
                    shared_collection.add(local_alpha, local_configuration, local_alpha_decomposition)
                    reseed_xor()
                self.reset_alpha_results(local_alpha_decomposition)
                next_permutation_function_ptr(local_configuration, self.atoms)

            free(local_configuration)
            free(local_alpha_decomposition)

        total = time.time()-t0

        structure_list = [self.configuration_to_structure(<uint8_t[:self.atoms]>shared_collection.get_configuration(i)) for i in range(shared_collection.size())]
        decomp_list = [self.alpha_to_dict(np.asarray(<double[:self.shell_count, :self.species_count, :self.species_count]>shared_collection.get_decomposition(i))) for i in range(shared_collection.size())]

        lps = total_iterations if iterations == 'all' else iterations

        return structure_list, decomp_list, lps, total/lps