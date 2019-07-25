from libc.stdint cimport uint8_t, uint32_t, uint64_t
from libc.string cimport memset
from libc.stdlib cimport malloc, free
from libc.math cimport fabs
from sqsgenerator.core.collection cimport ConfigurationCollection
from sqsgenerator.core.sqs cimport SqsIterator
from sqsgenerator.core.utils cimport next_permutation_lex, knuth_fisher_yates_shuffle, reseed_xor, unrank_permutation
cimport cython
cimport base
cimport openmp
from collections import Counter
from math import factorial
from cython.parallel import parallel
import numpy as np
import time
import multiprocessing

cdef extern from '<float.h>':
    cdef double DBL_MAX

ctypedef bint (*next_permutation_function) (uint8_t *configuration, size_t atoms) nogil

cdef class DosqsIterator(base.BaseIterator):

    cdef double[:, :, :] constant_factor_matrix
    cdef double *constant_factor_matrix_ptr
    cdef SqsIterator sqs_iterator

    def __cinit__(self, structure, dict mole_fractions, dict weights, verbosity=0):
        #super(SqsIterator, self).__cinit__(structure, mole_fractions, weights, verbosity=verbosity)
        self.constant_factor_matrix = self.make_constant_factor_matrix()
        self.constant_factor_matrix_ptr = <double*> &self.constant_factor_matrix[0, 0, 0]
        self.sqs_iterator = SqsIterator(structure, mole_fractions, weights, verbosity=verbosity)


    cdef double[:, :, :] make_constant_factor_matrix(self):
        cdef double[:, :, :] constant_factor_matrix = np.ascontiguousarray(np.zeros((self.atoms, self.atoms, 3)))

        cdef size_t i = 0, j = 0, k = 0, l = 0
        cdef double denominator
        cdef double vec_x, vec_y, vec_z, vec_sum
        cdef double value_x, value_y, value_z
        cdef double weight
        cdef int shell

        for i in range(self.atoms):
            for j in range(i, self.atoms):
                if i == j:
                    constant_factor_matrix[i, j, 0] = 0.0
                    constant_factor_matrix[i, j, 1] = 0.0
                    constant_factor_matrix[i, j, 2] = 0.0
                    continue
                if i != j:
                    shell = self.shell_number_matrix[i,j]
                    weight = self.weights[shell] if shell in self.weights else 0.0
                    denominator = (2 * self.shell_neighbor_mapping[shell] * self.atoms) if shell in self.weights else 0.0

                    vec_x, vec_y, vec_z = self.distance_vectors[i, j, 0], self.distance_vectors[i, j, 1], self.distance_vectors[i, j, 2]
                    vec_x, vec_y, vec_z = fabs(vec_x ** 2), fabs(vec_y ** 2), fabs(vec_z ** 2)
                    vec_sum = vec_x + vec_y + vec_z
                    value_x = ((vec_x / vec_sum) * 3.0 * weight / denominator) if shell in self.weights else 0.0
                    value_y = ((vec_y / vec_sum) * 3.0 * weight / denominator) if shell in self.weights else 0.0
                    value_z = ((vec_z / vec_sum) * 3.0 * weight / denominator) if shell in self.weights else 0.0

                    constant_factor_matrix[i, j, 0] = value_x
                    constant_factor_matrix[j, i, 0] = value_x

                    constant_factor_matrix[i, j, 1] = value_y
                    constant_factor_matrix[j, i, 1] = value_y

                    constant_factor_matrix[i, j, 2] = value_z
                    constant_factor_matrix[j, i, 2] = value_z

        return constant_factor_matrix

    def alpha_to_dict(self, double[:, :, :, :] alpha_decomposition):
        axis_mapping = {
            0: "x",
            1: "y",
            2: "z"
        }

        cdef int dimensions = 3

        species = list(self.mole_fractions.keys())
        rearranged_alphas = {}
        for i in range(dimensions):
            directional_rearranged_alphas = {}
            for j in range(self.species_count):
                for k in range(j, self.species_count):
                    if j != k:
                        alphas = []
                        for l in range(self.shell_count):
                            alphas.append(alpha_decomposition[i, l, j, k] + alpha_decomposition[i, l, k, j])
                        directional_rearranged_alphas['{0}-{1}'.format(species[j], species[k])] = alphas
            rearranged_alphas[axis_mapping[i]] = directional_rearranged_alphas

        return rearranged_alphas

    @cython.boundscheck(False)
    @cython.wraparound(False)
    @cython.cdivision(True)
    cdef double calculate_parameter(self, uint8_t* configuration, double *constant_factor_matrix, double* alpha_decomposition, int dimensions, double main_sum_weight, double *anisotropic_weight) nogil:
        cdef size_t i = 0, j = 0, k = 0, l = 0
        cdef int current_species = -1, compare_species = -1, shell = -1
        cdef double alpha[3]
        cdef double current_bond_ratio_x
        cdef double current_bond_ratio_y
        cdef double current_bond_ratio_z
        cdef double current_alpha
        cdef double denominator
        cdef double objective = 0
        cdef double current_directional_bond_ratio
        cdef int d1 = self.shell_count*self.species_count*self.species_count, d2 = self.species_count*self.species_count, d3 = self.species_count

        alpha[0] = 0.0
        alpha[1] = 0.0
        alpha[2] = 0.0

        for i in range(self.shell_count):
            compare_species = configuration[i]
            for j in range(i, self.atoms):
                current_species = configuration[j]
                shell = self.shell_number_matrix[i,j] - 1
                if current_species != compare_species and -1 < shell <= self.shell_count-1:

                    denominator = (self.mole_fractions_ptr[current_species]*self.mole_fractions_ptr[compare_species])
                    current_bond_ratio_x = alpha_decomposition[shell*d2 + compare_species*d3 + current_species]
                    current_bond_ratio_y = alpha_decomposition[d1 + shell*d2 + compare_species*d3 + current_species]
                    current_bond_ratio_z = alpha_decomposition[2*d1 + shell*d2 + compare_species*d3 + current_species]

                    current_bond_ratio_x += constant_factor_matrix[i * self.atoms + j] / denominator
                    current_bond_ratio_y += constant_factor_matrix[i * self.atoms + j + 1] / denominator
                    current_bond_ratio_z += constant_factor_matrix[i * self.atoms + j + 2] / denominator

                    alpha_decomposition[shell*d2 + compare_species*d3 + current_species] = current_bond_ratio_x
                    alpha_decomposition[d1 + shell*d2 + compare_species*d3 + current_species] = current_bond_ratio_y
                    alpha_decomposition[2*d1 + shell*d2 + compare_species*d3 + current_species] = current_bond_ratio_z

                    alpha_decomposition[shell*d2 + current_species*d3 + compare_species] = current_bond_ratio_x
                    alpha_decomposition[d1 + shell*d2 + current_species*d3 + compare_species] = current_bond_ratio_y
                    alpha_decomposition[2*d1 + shell*d2 + current_species*d3 + compare_species] = current_bond_ratio_z

        # Reduce results
        for i in range(dimensions):
            for j in range(self.shell_count):
                for k in range(self.species_count):
                    for l in range(self.species_count):
                        if k != l:
                            current_directional_bond_ratio = alpha_decomposition[d1*i + d2*j + d3*k + l]
                            current_alpha = fabs(self.weights_ptr[j]/2 - current_directional_bond_ratio)
                            alpha_decomposition[d1*i + d2*j + d3*k + l] = current_alpha
                            alpha[i] = alpha[i] + current_alpha


        objective += (fabs(alpha[0]) + fabs(alpha[1]) + fabs(alpha[2]))*main_sum_weight
        if dimensions == 2:
            objective += fabs(alpha[0] - alpha[1])*anisotropic_weight[0]
        if dimensions == 3:
            objective += (
                fabs(alpha[0] - alpha[1])*anisotropic_weight[0] + #alpha_x-alpha_y
                fabs(alpha[0] - alpha[2])*anisotropic_weight[1] + #alpha_x-alpha_z
                fabs(alpha[1] - alpha[2])*anisotropic_weight[1]   #alpha_y-alpha_z
            )

        return objective

    def calculate_alpha(self, double main_sum_weight, list anisotropic_weights):
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
        cdef double[:, :, :, :] alpha_decomposition = np.ascontiguousarray(np.zeros((3, self.shell_count, self.species_count, self.species_count)))
        cdef double *alpha_decomposition_ptr = <double*> &alpha_decomposition[0, 0, 0, 0]
        cdef double[:] anisotropy_weights = np.ascontiguousarray(anisotropic_weights)
        cdef double *anisotropy_weights_ptr = <double*> &anisotropy_weights[0]
        cdef uint8_t[:] configuration = self.configuration_from_structure()
        cdef uint8_t *configuraton_ptr = <uint8_t*> &configuration[0]

        self.reset_alpha_results(alpha_decomposition_ptr)
        alpha = self.calculate_parameter(configuraton_ptr, self.constant_factor_matrix_ptr, alpha_decomposition_ptr, 3,  main_sum_weight, anisotropy_weights_ptr)

        rearranged_alphas = self.alpha_to_dict(alpha_decomposition)

        self.species_count = old_species_count
        return rearranged_alphas

    def sort_numpy(self, uint8_t[:] a, kind='quick'):
        np.asarray(a).sort(kind=kind)

    def iteration(self, double main_sum_weight, list anisotropic_weights, output_structures=10, iterations=100000):
        cdef double dosqs_alpha
        cdef double sqs_best_alpha
        cdef double sqs_alpha
        cdef int dimensions = 3
        cdef uint64_t c_iterations
        cdef bint all_flag = iterations == 'all'
        cdef bint all_output_structures_flag = output_structures == 'all'
        cdef ConfigurationCollection shared_collection
        cdef double[:] dosqs_anisotropy_weights = np.ascontiguousarray(anisotropic_weights)
        cdef double *dosqs_anisotropy_weights_ptr = <double*> &dosqs_anisotropy_weights[0]
        #Ensure decomposition are contigous in memory
        cdef double *dosqs_alpha_decomposition = <double*> malloc(sizeof(double)*3*self.shell_count*self.species_count*self.species_count)
        cdef double *sqs_alpha_decomposition = <double*> malloc(sizeof(double)*self.shell_count*self.species_count*self.species_count)

        shared_collection = ConfigurationCollection(output_structures if not all_output_structures_flag else 0, self.atoms, self.shell_count, self.species_count, dimension=3)

        cdef size_t i = 0

        # dimensions 0 = x, 1 = y, 2 = z
        self.reset_alpha_results(dosqs_alpha_decomposition)

        if iterations == 'all':
            self.sort_numpy(self.configuration)
            composition = dict(Counter(np.asarray(self.configuration)))

            total_iterations = factorial(self.atoms)
            current_iteration = 0
            for species, amount in composition.items():
                total_iterations = total_iterations/factorial(amount)
            print('Configurations to check: {0}'.format(total_iterations))
            c_iterations = total_iterations
        else:
            c_iterations = iterations

        if all_flag:
            next_permutation_function_ptr = next_permutation_lex
        else:
            next_permutation_function_ptr = knuth_fisher_yates_shuffle

        if all_flag:
            unrank_permutation(self.configuration_ptr, self.atoms, self.composition_hist_ptr, self.species_count, c_iterations, 1)
        else:
            knuth_fisher_yates_shuffle(self.configuration_ptr, self.atoms)

        t0 = time.time()
        for i in range(c_iterations):
            dosqs_alpha = fabs(self.calculate_parameter(self.configuration_ptr, self.constant_factor_matrix_ptr, dosqs_alpha_decomposition, dimensions, main_sum_weight, dosqs_anisotropy_weights_ptr))

            if dosqs_alpha <= shared_collection.best_objective():
                shared_collection.add(dosqs_alpha, self.configuration_ptr, dosqs_alpha_decomposition)
                reseed_xor()
            self.reset_alpha_results(dosqs_alpha_decomposition)
            next_permutation_function_ptr(self.configuration_ptr, self.atoms)
        total = time.time() - t0


        structure_list = [self.configuration_to_structure(<uint8_t[:self.atoms]>shared_collection.get_configuration(i)) for i in range(shared_collection.size())]
        decomp_list = [self.alpha_to_dict(np.asarray(<double[:3, :self.shell_count, :self.species_count, :self.species_count]>shared_collection.get_decomposition(i))) for i in range(shared_collection.size())]

        lps = total_iterations if iterations == 'all' else iterations
        for i in range(shared_collection.size()):
            print('LIST AFTER: {}/{} {}'.format(i, shared_collection.size(), np.array(<uint8_t[:self.atoms]>shared_collection.get_configuration(i))))

        return structure_list, decomp_list, lps, total/lps

    @cython.boundscheck(False)
    @cython.wraparound(False)
    cdef void reset_alpha_results(self, double* alpha_decomposition) nogil:
        memset(alpha_decomposition, 0, sizeof(double) * self.shell_count * self.species_count * self.species_count * 3)

cdef class ParallelDosqsIterator(DosqsIterator):

    cdef size_t num_threads

    def __cinit__(self, structure, dict mole_fractions, dict weights, verbosity=0, num_threads=multiprocessing.cpu_count()):
        self.num_threads = num_threads

    def iteration(self, double main_sum_weight, list anisotropic_weights, iterations=100000, output_structures=10):
        cdef int num_threads = self.num_threads
        cdef int thread_id
        cdef int dimensions = 3
        cdef bint all_flag = iterations == 'all'
        cdef bint all_output_structures_flag = output_structures == 'all'
        cdef uint64_t permutations
        cdef uint64_t start_permutation
        cdef uint64_t end_permutation
        cdef uint64_t c_iterations

        cdef Py_ssize_t i = 0, j = 0, k = 0

        cdef double local_sqs_alpha
        cdef double local_best_sqs_alpha
        cdef double local_dosqs_alpha
        cdef uint8_t* local_configuration
        cdef double* local_dosqs_alpha_decomposition
        cdef next_permutation_function next_permutation_function_ptr
        cdef ConfigurationCollection shared_collection

        shared_collection = ConfigurationCollection(output_structures if not all_output_structures_flag else 0, self.atoms, self.shell_count, self.species_count, dimension=3)

        openmp.omp_set_num_threads(self.num_threads)
        print('Threads used: {}'.format(self.num_threads))

        cdef double[:] dosqs_anisotropy_weights = np.array(anisotropic_weights)
        cdef double *dosqs_anisotropy_weights_ptr = <double*> &dosqs_anisotropy_weights[0]

        if iterations == 'all':
            self.sort_numpy(self.configuration)
            composition = dict(Counter(np.asarray(self.configuration)))
            total_iterations = factorial(self.atoms)
            current_iteration = 0
            for species, amount in composition.items():
                total_iterations = total_iterations/factorial(amount)
            print('Configurations to check: {0}'.format(int(total_iterations)))
            permutations = total_iterations
        else:
            c_iterations = iterations

        t0 = time.time()
        with nogil, parallel():

            thread_id = openmp.omp_get_thread_num()
            local_configuration = <uint8_t*>malloc(sizeof(uint8_t)*self.atoms)
            local_dosqs_alpha_decomposition = <double*>malloc(sizeof(double)*3*self.shell_count*self.species_count*self.species_count)

            for i in range(self.atoms):
                local_configuration[i] = self.configuration[i]
            self.reset_alpha_results(local_dosqs_alpha_decomposition)

            start_permutation = thread_id*(permutations if all_flag else c_iterations)/num_threads
            end_permutation = (thread_id+1)*(permutations if all_flag else c_iterations)/num_threads+1

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

            for k in range(start_permutation, end_permutation):
                local_dosqs_alpha = fabs(self.calculate_parameter(local_configuration, self.constant_factor_matrix_ptr, local_dosqs_alpha_decomposition, dimensions, main_sum_weight, dosqs_anisotropy_weights_ptr))
                if local_dosqs_alpha <= shared_collection.best_objective():
                    shared_collection.add(local_dosqs_alpha, local_configuration, local_dosqs_alpha_decomposition)
                    reseed_xor()
                self.reset_alpha_results(local_dosqs_alpha_decomposition)
                next_permutation_function_ptr(local_configuration, self.atoms)


            free(local_configuration)

        total = time.time()-t0

        structure_list = [self.configuration_to_structure(<uint8_t[:self.atoms]>shared_collection.get_configuration(i)) for i in range(shared_collection.size())]
        decomp_list = [self.alpha_to_dict(np.asarray(<double[:3, :self.shell_count, :self.species_count, :self.species_count]>shared_collection.get_decomposition(i))) for i in range(shared_collection.size())]
        dl = [<double[:3, :self.shell_count, :self.species_count, :self.species_count]>shared_collection.get_decomposition(i) for i in range(shared_collection.size())]

        lps = total_iterations if iterations == 'all' else iterations

        return structure_list, decomp_list, lps, total/lps