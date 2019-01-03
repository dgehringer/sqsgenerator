#include <gmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void permutation_count_mpz(mpz_t mi_result, uint8_t *configuration, size_t atoms);
uint64_t rank_permutation(uint8_t *configuration, size_t atoms, size_t species);
void rank_permutation_mpz(mpz_t result, uint8_t *configuration, size_t atoms, size_t species);
void unrank_permutation_mpz(uint8_t *configuration, size_t atoms, size_t *hist, size_t species, mpz_t mi_permutations, mpz_t mi_rank);
void unrank_permutation(uint8_t *configuration, size_t atoms, size_t *hist, size_t species, uint64_t permutations, uint64_t rank);
bool next_permutation_lex(uint8_t *configuration, size_t atoms);
void rank_permutation_mpz(mpz_t result, uint8_t *configuration, size_t atoms, size_t species);
size_t configuration_species_count(uint8_t *configuration, size_t atoms);