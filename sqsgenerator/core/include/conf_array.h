#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <gmp.h>
#include "rank.h"

typedef struct __conf_array_struct {
    size_t max_size;
    size_t size;
    size_t atoms;
    size_t alpha_decomp_size;
    double* objective;
    double* alpha_decomp;
    uint8_t* data;
    bool* set_flags;
    double best_objective;
    uint64_t species_count;
    mpz_t *ranks;
    pthread_mutex_t mutex;
} conf_array_t;

conf_array_t* conf_array_init(size_t max_size, size_t atoms, size_t decomp_size);
uint8_t* conf_array_get_conf(conf_array_t* array, size_t index);
double* conf_array_get_decomp(conf_array_t* array, size_t index);
double conf_array_get_objective(conf_array_t* array, size_t index);
bool conf_array_add(conf_array_t* array, double objective, uint8_t* configuration, double* decomp);
void conf_array_destroy(conf_array_t* array);
