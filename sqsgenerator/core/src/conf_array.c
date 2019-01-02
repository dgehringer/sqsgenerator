
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "conf_array.h"


/* Locks the arrays mutex only if the lock was not acquired */
void conf_array_acquire_mutex(conf_array_t* a){
    pthread_mutex_lock(&(a->mutex));
}
/* Releases the arrays mutex only if the lock was acquired */
void conf_array_release_mutex(conf_array_t* a){
    pthread_mutex_unlock(&(a->mutex));
}


void __conf_array_clear_internal(conf_array_t* array){
    uint8_t* d = array->data;
    for (size_t i = 0; i < array->max_size; i++) {
        array->objective[i] = DBL_MAX;
        array->set_flags[i] = false;
        mpz_set_ui(array->ranks[i], 0);
    }
    memset(d, 0, sizeof(uint8_t) * array->atoms * array->max_size);
    array->size = 0;
}

void conf_array_clear(conf_array_t* array){
    conf_array_acquire_mutex(array);
    __conf_array_clear_internal(array);
    conf_array_release_mutex(array);
}

conf_array_t* conf_array_init(size_t max_size, size_t atoms, size_t decomp_size){
    conf_array_t* a = malloc(sizeof(conf_array_t));
    uint8_t* d = malloc(sizeof(uint8_t) * atoms * max_size);
    bool* flags = malloc(sizeof(bool) * max_size);
    double* obj = malloc(sizeof(double) * max_size);
    double* decomp = malloc(sizeof(double) * max_size * decomp_size);
    mpz_t* r = malloc(sizeof(mpz_t) * max_size);
    for (size_t i = 0; i < max_size; i++) {
        mpz_init(r[i]);
        mpz_set_ui(r[i], 0);
    }
    pthread_mutex_init(&(a->mutex), NULL);
    a->species_count = 0;
    a->ranks = r;
    a->max_size = max_size;
    a->size = 0;
    a->atoms = atoms;
    a->data = d;
    a->set_flags = flags;
    a->objective = obj;
    a->best_objective = DBL_MAX;
    a->alpha_decomp_size = decomp_size;
    a->alpha_decomp = decomp;
    conf_array_clear(a);
    return a;
}


void __conf_array_set_internal(conf_array_t* array, size_t index, double objective, uint8_t* conf, double* decomp){
    if (index < array->max_size) {
        //Count number of species in the configuration if it was not done before
        memcpy(&(array->data[index*array->atoms]), conf, sizeof(uint8_t)*array->atoms);
        memcpy(&(array->alpha_decomp[index*(array->alpha_decomp_size)]), decomp, sizeof(double)*array->alpha_decomp_size);
        array->objective[index] = objective;
        array->set_flags[index] = true;
        //Rank permutation
        rank_permutation_mpz(array->ranks[index], conf, array->atoms, array->species_count);
    }
}

void conf_array_set(conf_array_t* array, size_t index, double objective, uint8_t* conf, double* decomp){
    conf_array_acquire_mutex(array);
    __conf_array_set_internal(array, index, objective, conf, decomp);
    conf_array_release_mutex(array);
}

uint8_t* conf_array_get_conf(conf_array_t* array, size_t index){
    conf_array_acquire_mutex(array);
    uint8_t *result;
    if (index < array->size) {
        result =  &(array->data[index*array->atoms]);
    }
    else {
        result =  NULL;
    }
    conf_array_release_mutex(array);
    return result;
}
double* conf_array_get_decomp(conf_array_t* array, size_t index){
    conf_array_acquire_mutex(array);
    double *result;
    if (index < array->size) {
        result =  &(array->alpha_decomp[index*(array->alpha_decomp_size)]);
    }
    else {
        result =  NULL;
    }
    conf_array_release_mutex(array);
    return result;
}

double conf_array_get_objective(conf_array_t* array, size_t index){
    conf_array_acquire_mutex(array);
    double result;
    if (index < array->size) {
        result =  array->objective[index];
    }
    else {
        result = -DBL_MAX;
    }
    conf_array_release_mutex(array);
    return result;
}

void conf_array_destroy(conf_array_t* array){
    conf_array_acquire_mutex(array);
    for(size_t i = 0; i < array->max_size; i++){
        mpz_clear(array->ranks[i]);
    }
    free(array->set_flags);
    free(array->data);
    free(array->objective);
    free(array->ranks);
    conf_array_release_mutex(array);
    pthread_mutex_destroy(&(array->mutex));
    free(array);
}

int conf_array_available(conf_array_t* array){
    for (size_t i = 0; i < array->max_size; i++) {
        if (!array->set_flags[i]) {
            return i;
        }
    }
    return -1;
}

bool conf_array_add(conf_array_t* array, double objective, uint8_t* conf, double* decomp){
    conf_array_acquire_mutex(array);
    //New configuration is better than anything we had before
    if(objective < array->best_objective){
        array->best_objective = objective;
        //Avoid reacquiring lock
        __conf_array_clear_internal(array);
    }
    //New objective is bad =(
    if(objective > array->best_objective) {
        conf_array_release_mutex(array);
        return false;
    }
    if (array->species_count <= 0) {
        array->species_count = configuration_species_count(conf, array->atoms);
    }
    //Check if this configuration is already stored
    if(objective == array->best_objective) {
        //Check if configuration is already there
        mpz_t current_rank;
        mpz_init(current_rank);
        rank_permutation_mpz(current_rank, conf, array->atoms, array->species_count);
        for (size_t i = 0; i < array->size; i++) {
            if(mpz_cmp(current_rank, array->ranks[i]) == 0) {
                 mpz_clear(current_rank);
                 conf_array_release_mutex(array);
                 return false;
            }
        }
    }

    //Here if the new objective is smaller of if its EQUAL
    int available_index = conf_array_available(array);
    if (available_index >= 0) {
        __conf_array_set_internal(array, (size_t)available_index, objective, conf, decomp);
        array->size = (available_index+1);
        conf_array_release_mutex(array);
        return true;
    }
    conf_array_release_mutex(array);
    return false;
}
