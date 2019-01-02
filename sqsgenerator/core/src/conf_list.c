#include "conf_list.h"
#include <float.h>
#include <string.h>

void conf_list_destroy_element(void *data, void *meta_data){
    node_conf_data_t* node_data = (node_conf_data_t*)data;
    mpz_clear(node_data->rank);
    free(node_data->alpha_decomp);
    free(node_data->configuration);
    free(node_data);
}

node_conf_data_t* conf_list_create_data_struct(conf_list_t* l, double alpha, uint8_t* conf, double* decomp){
    node_conf_data_t* node_data = malloc(sizeof(node_conf_data_t));
    if (node_data) {
        /* Create a copy of the array */
        uint8_t* conf_ptr = malloc(sizeof(uint8_t)*l->atoms);
        if (!conf_ptr) {
            return NULL;
        }
        double* decomp_ptr = malloc(sizeof(double)*l->alpha_decomp_size);
        if (!decomp_ptr) {
            return NULL;
        }
        memcpy(conf_ptr, conf, sizeof(uint8_t)*l->atoms);
        memcpy(decomp_ptr, decomp, sizeof(double)*l->alpha_decomp_size);
        node_data->alpha_decomp = decomp_ptr;
        node_data->alpha = alpha;
        node_data->configuration = conf_ptr;
        mpz_init(node_data->rank);
        rank_permutation_mpz(node_data->rank, conf, l->atoms, l->species_count);
        return node_data;
    }
    return NULL;
}

conf_list_t* conf_list_init(size_t atoms, size_t decomp_size){
    conf_list_t* l = malloc(sizeof(conf_list_t));
    if(l){
        l->best_objective = DBL_MAX;
        l->atoms = atoms;
        l->alpha_decomp_size = decomp_size;
        l->species_count = 0;
        list_t* inner = list_init(conf_list_destroy_element);
        if (inner) {
            l->__inner_list = inner;
        }
    }
    return l;
}

bool conf_list_add(conf_list_t* l, double alpha, uint8_t *conf, double* decomp){
    list_acquire_mutex(l->__inner_list);

    if (alpha < l->best_objective) {
        l->best_objective = alpha;
        __list_clear_internal(l->__inner_list);
        l->size  = 0;
    }

    if (alpha > l->best_objective){
        list_release_mutex(l->__inner_list);
        return false;
    }

    if (l->species_count <= 0) {
        l->species_count = configuration_species_count(conf, l->atoms);
    }

    if(alpha == l->best_objective) {
    //Check if configuration is already there
        mpz_t current_rank;
        mpz_init(current_rank);
        node_t *current;
        node_conf_data_t *current_data;
        rank_permutation_mpz(current_rank, conf, l->atoms, l->species_count);
        for (size_t i = 0; i < l->__inner_list->size; i++) {
            current = __list_get_node_internal(l->__inner_list, i);
            current_data = current->data;
            if(mpz_cmp(current_rank, current_data->rank) == 0) {
                 mpz_clear(current_rank);
                 list_release_mutex(l->__inner_list);
                 return false;
            }
        }
    }

    node_conf_data_t* data = conf_list_create_data_struct(l, alpha, conf, decomp);
    if (data) {
        bool result = __list_append_internal(l->__inner_list, data, NULL);
        if (result) {
            l->size = l->__inner_list->size;
        }
        list_release_mutex(l->__inner_list);
            return result;
    }
    list_release_mutex(l->__inner_list);
    return false;
}

double conf_list_get_objective(conf_list_t* l, size_t index){
    node_conf_data_t* data = (node_conf_data_t*) list_get_data(l->__inner_list, index);
    if (data) {
        return data->alpha;
    }
    return -DBL_MAX;
}

uint8_t *conf_list_get_conf(conf_list_t* l, size_t index){
    node_conf_data_t* data = (node_conf_data_t*) list_get_data(l->__inner_list, index);
    if (data) {
        return data->configuration;
    }
    return NULL;
}

double* conf_list_get_decomp(conf_list_t* l, size_t index){
    node_conf_data_t* data = (node_conf_data_t*) list_get_data(l->__inner_list, index);
    if (data) {
        return data->alpha_decomp;
    }
    return NULL;
}

void conf_list_destroy(conf_list_t* l){
    if (l) {
        list_destroy(l->__inner_list);
        free(l);
    }
}
