#include <gmp.h>
#include "rank.h"



int spec_comparator(const void *a, const void *b)
{
    uint8_t va = *(uint8_t*) a;
    uint8_t vb = *(uint8_t*) b;
    return va - vb;
}

size_t configuration_species_count(uint8_t *configuration, size_t atoms) {
    //Sort configuration first
    uint8_t *sorted_configuration = malloc(sizeof(uint8_t) * atoms);
    memcpy(sorted_configuration, configuration, sizeof(uint8_t) * atoms);
    qsort(sorted_configuration, atoms, sizeof(uint8_t), spec_comparator);

    size_t unique = 1; //incrase we have only one element; it is unique!
    for (size_t i = 0; i < atoms - 1; i++) {
        if (sorted_configuration[i] == sorted_configuration[i + 1])
            continue;
        else
            unique++;
    }
    free(sorted_configuration);
    //fprintf(stderr, "\nATOMS: %d\n", atoms);
    return unique;
}

size_t *configuration_histogram(uint8_t *configuration, size_t atoms) {
    //Sort configuration first
    uint8_t *sorted_configuration = malloc(sizeof(uint8_t) * atoms);
    memcpy(sorted_configuration, configuration, sizeof(uint8_t) * atoms);
    qsort(sorted_configuration, atoms, sizeof(uint8_t), spec_comparator);

    size_t unique = configuration_species_count(configuration, atoms);
    size_t *histogram = malloc(sizeof(size_t) * unique);
    unique = 0;
    histogram[unique] = 1;
    for(int i = 0; i < atoms-1; i++) {
        if (sorted_configuration[i] == sorted_configuration[i + 1])
            histogram[unique]++;
        else
            histogram[++unique] = 1;
    }
    free(sorted_configuration);
    return histogram;
}

void permutation_count_mpz(mpz_t mi_result, uint8_t *configuration, size_t atoms) {
    mpz_t mi_permutations;
    mpz_t mi_species_permutations;

    mpz_init(mi_permutations);
    mpz_init(mi_species_permutations);

    size_t *histogram = configuration_histogram(configuration, atoms);

    size_t species = configuration_species_count(configuration, atoms);

    factorial_mpz(mi_permutations, atoms);

    for (size_t i = 0; i < species; i++) {
        factorial_mpz(mi_species_permutations, histogram[i]);
        mpz_cdiv_q(mi_permutations, mi_permutations, mi_species_permutations);
    }
    mpz_init_set(mi_result, mi_permutations);
}



void rank_permutation_mpz(mpz_t result, uint8_t *configuration, size_t atoms, size_t species) {
    mpz_t mi_rank;
    mpz_t mi_suffix_permutations;
    mpz_t mi_temp;

    unsigned long int _hist[species];
    size_t i, j;
    uint8_t _x;

    for (i = 0; i < species; i++) {
        _hist[i] = 0;
    }

    //Initialize mpz variables
    mpz_init(mi_rank);
    mpz_init(mi_suffix_permutations);
    mpz_init(mi_temp);
    mpz_set_ui(mi_rank, 1);
    mpz_set_ui(mi_suffix_permutations, 1);

    for (size_t i = 0; i < atoms; i++) {
        _x = configuration[((atoms - 1) - i)];
        _hist[_x]++;
        for (j = 0; j < species; j++) {
            if (j < _x) {
                mpz_mul_ui(mi_temp, mi_suffix_permutations, _hist[j]);
                mpz_cdiv_q_ui(mi_temp, mi_temp, _hist[_x]);
                mpz_add(mi_rank, mi_rank, mi_temp);
            }
        }
        mpz_mul_ui(mi_temp, mi_suffix_permutations, (i + 1));
        mpz_cdiv_q_ui(mi_suffix_permutations, mi_temp, _hist[_x]);
    }
    mpz_set(result, mi_rank);
    mpz_clear(mi_rank);
    mpz_clear(mi_suffix_permutations);
    mpz_clear(mi_temp);

}

uint64_t rank_permutation(uint8_t *configuration, size_t atoms, size_t species) {
    mpz_t mi_result;
    mpz_init(mi_result);
    rank_permutation_mpz(mi_result, configuration, atoms, species);
    return (uint64_t)mpz_get_ui(mi_result);
}

void unrank_permutation_mpz(uint8_t *configuration, size_t atoms, size_t *hist, size_t species, mpz_t mi_permutations, mpz_t mi_rank) {
    int k = 0;
    size_t _hist[species];
    uint64_t _configuration[atoms];
    mpz_t _mi_suffixcount;
    mpz_t _mi_permutations;
    mpz_t _mi_rank;
    mpz_t _mi_help;

    /* Initialize helpers */
    mpz_init(_mi_suffixcount);
    mpz_init(_mi_help);
    mpz_init_set(_mi_permutations, mi_permutations);
    mpz_init_set(_mi_rank, mi_rank);

    for (size_t i = 0; i < atoms; i++) {
        _configuration[i] = 0;
    }
    for (size_t j = 0; j < species; j++) {
        _hist[j] = hist[j];
    }

    /* Return if permutation rank is bigger than the number of perms */
    if (mpz_cmp(_mi_permutations, _mi_rank) < 0) {
        return;
    }

    for (size_t i = 0; i < atoms; i++) {
        for (size_t j = 0; j < species; j++) {
            /* mi_suffixcount is the number of distinct permutations that 
             * begin with j */
            mpz_mul_si(_mi_suffixcount, _mi_permutations, _hist[j]);
            mpz_set_si(_mi_help, atoms - i);
            mpz_cdiv_q(_mi_suffixcount, _mi_suffixcount, _mi_help);

            if (mpz_cmp(_mi_rank, _mi_suffixcount) <= 0) {
                _configuration[k] = j;
                mpz_set(_mi_permutations, _mi_suffixcount);
                _hist[j]--;
                k++;
                if (_hist[j] == 0) {
                    j++;
                }
                break;
            }
            mpz_sub(_mi_rank, _mi_rank, _mi_suffixcount);
        }
    }

    /* Copy local configuration */
    for (size_t i = 0; i < atoms; i++) {
        configuration[i] = _configuration[i];
    }

    /* Clean up */
    mpz_clear(_mi_suffixcount);
    mpz_clear(_mi_permutations);
    mpz_clear(_mi_rank);
    mpz_clear(_mi_help);
}

void unrank_permutation(uint8_t *configuration, size_t atoms, size_t *hist, size_t species, uint64_t permutations, uint64_t rank) {
    mpz_t mi_permutations;
    mpz_t mi_rank;

    mpz_init_set_ui(mi_permutations, permutations);
    mpz_init_set_ui(mi_rank, rank);

    unrank_permutation_mpz(configuration, atoms, hist, species, mi_permutations, mi_rank);

    mpz_clear(mi_permutations);
    mpz_clear(mi_rank);
}

bool next_permutation_lex(uint8_t *configuration, size_t atoms) {
    size_t k = atoms - 2;
    size_t l = atoms - 1;
    int m, n;
    size_t length;
    uint8_t temporary;

    while (configuration[k] >= configuration[k + 1]) {
        k -= 1;
        if (k < 0) {
            return false;
        }
    }

    while (configuration[k] >= configuration[l]) l -= 1;

    temporary = configuration[k];
    configuration[k] = configuration[l];
    configuration[l] = temporary;

    length = atoms - (k + 1);

    for (size_t i = 0; i < length/2; i++) {
        m = k + i + 1;
        n = atoms - i - 1;
        temporary = configuration[m];
        configuration[m] = configuration[n];
        configuration[n] = temporary;
    }

    return true;
}