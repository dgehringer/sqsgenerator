//
// Created by dominik on 21.05.21.
//

#ifndef SQSGENERATOR_RANK_H
#define SQSGENERATOR_RANK_H

#include <gmp.h>
#include <types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <gmpxx.h>
#include <unordered_map>

using namespace sqsgenerator;

mpz_class total_permutations(const Configuration &conf);
const Configuration unique_species(Configuration conf);
std::vector<size_t> configuration_histogram(const Configuration &conf);

mpz_class rank_permutation_gmp(const Configuration &conf, size_t nspecies);
uint64_t rank_permutation_std(const Configuration &conf, size_t nspecies);

void unrank_permutation_gmp(Configuration &conf, std::vector<size_t> hist, mpz_class total_permutations, mpz_class rank);
void unrank_permutation_std(Configuration &conf, std::vector<size_t> hist, uint64_t total_permutations, uint64_t rank);

#endif //SQSGENERATOR_RANK_H
