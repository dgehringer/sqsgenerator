#include "conf_collection.h"
#include <float.h>
#include <string.h>
#include <stdio.h>

conf_collection_t* conf_collection_init(size_t max_size, size_t atoms, size_t decomp_size){
    conf_collection_t* collection = malloc(sizeof(conf_collection_t));
    if (collection) {
        if (max_size > 0) {
            conf_array_t* array = conf_array_init(max_size, atoms, decomp_size);
            if (array) {
                collection->__inner_array = array;
                collection->__inner_list = NULL;
            }
            else {
                return NULL;
            }
        }
        else {
            conf_list_t* list = conf_list_init(atoms, decomp_size);
            if (list) {
                collection->__inner_list = list;
                collection->__inner_array = NULL;
            }
            else {
                return NULL;
            }
        }
        collection->size = 0;
        collection->best_objective = DBL_MAX;
    }
    return collection;
}

uint8_t * conf_collection_get_conf(conf_collection_t* c, size_t index) {
    if (c->__inner_array) {
        return conf_array_get_conf(c->__inner_array, index);
    }
    else {
        return conf_list_get_conf(c->__inner_list, index);
    }
}

double* conf_collection_get_decomp(conf_collection_t* c, size_t index){
    if (c->__inner_array) {
        return conf_array_get_decomp(c->__inner_array, index);
    }
    else {
        return conf_list_get_decomp(c->__inner_list, index);
    }
}

double conf_collection_get_objective(conf_collection_t* c, size_t index){
    if (c->__inner_array) {
        return conf_array_get_objective(c->__inner_array, index);
    }
    else {
        return conf_list_get_objective(c->__inner_list, index);
    }
}

bool conf_collection_add(conf_collection_t* c, double objective, uint8_t *conf, double* decomp){
    bool result;
    if (c->__inner_array) {
        result = conf_array_add(c->__inner_array, objective, conf, decomp);
        if (result) {
            c->best_objective = c->__inner_array->best_objective;
            c->size = c->__inner_array->size;
        }
    }
    else {
        result = conf_list_add(c->__inner_list, objective, conf, decomp);
        if (result) {
            c->best_objective = c->__inner_list->best_objective;
            c->size = c->__inner_list->size;
        }
    }
    return result;
}
void conf_collection_destroy(conf_collection_t* c){
    if (c) {
        if (c->__inner_array) {
            conf_array_destroy(c->__inner_array);
        }
        else {
            conf_list_destroy(c->__inner_list);
        }
        free(c);
    }

}
