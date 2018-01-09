
#include <gmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

void factorial_mpz(mpz_t mi_result, uint64_t n);
uint32_t xor32();
uint32_t xor64();
uint32_t xor96();
uint32_t xor128();
uint32_t xor160();
uint32_t xorwow();
uint32_t rand_int(uint32_t n);
void reseed_xor();
bool knuth_fisher_yates_shuffle(uint8_t *configuration, size_t atoms);