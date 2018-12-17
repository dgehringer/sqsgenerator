
#include <float.h>
#include <string.h>
#include "conf_array.h"

/* Locks the arrays mutex only if the lock was not acquired */
void conf_array_acquire_mutex(conf_array_t* a){
    if(!a->lock_acquired) {
        pthread_mutex_lock(&(a->mutex));
        a->lock_acquired = true;
    }
}
/* Releases the arrays mutex only if the lock was acquired */
void conf_array_release_mutex(conf_array_t* a){
    if(a->lock_acquired){
        a->lock_acquired = false;
        pthread_mutex_unlock(&(a->mutex));
    }
}

void conf_array_clear(conf_array_t* array){
    conf_array_acquire_mutex(array);
    uint8_t* d = array->data;
    size_t atoms = array->atoms;
    for (size_t i = 0; i < array->max_size; i++) {
        array->objective[i] = DBL_MAX;
        array->set_flags[i] = false;
    }
    memset(d, 0, sizeof(uint8_t) * array->atoms * array->max_size);
    array->size = 0;
    conf_array_release_mutex(array);
}

conf_array_t* conf_array_init(size_t max_size, size_t atoms, size_t decomp_size){
    conf_array_t* a = malloc(sizeof(conf_array_t));
    uint8_t* d = malloc(sizeof(uint8_t) * atoms * max_size);
    bool* flags = malloc(sizeof(bool) * max_size);
    double* obj = malloc(sizeof(double) * max_size);
    double* decomp = malloc(sizeof(double) * max_size * decomp_size);
    pthread_mutex_init(&(a->mutex), NULL);
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

void conf_array_set(conf_array_t* array, size_t index, double objective, uint8_t* conf, double* decomp){
    conf_array_acquire_mutex(array);
    if (index < array->max_size) {
        memcpy(&(array->data[index*array->atoms]), conf, sizeof(uint8_t)*array->atoms);
        memcpy(&(array->alpha_decomp[index*(array->alpha_decomp_size)]), decomp, sizeof(double)*array->alpha_decomp_size);
        array->objective[index] = objective;
        array->set_flags[index] = true;
    }
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
    free(array->set_flags);
    free(array->data);
    free(array->objective);
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
    if(objective < array->best_objective){
        array->best_objective = objective;
        conf_array_clear(array);
    }
    if(objective >= array->best_objective) {
        conf_array_release_mutex(array);
        return false;
    }
    int available_index = conf_array_available(array);
    if (available_index >= 0) {
        conf_array_set(array, (size_t)available_index, objective, conf, decomp);
        array->size = (available_index+1);
        conf_array_release_mutex(array);
        return true;
    }
    conf_array_release_mutex(array);
    return false;
}
