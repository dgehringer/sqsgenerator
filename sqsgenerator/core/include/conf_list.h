#include <stdlib.h>
#include <stdint.h>
#include "list.h"

#define conf_list_size(l) (l->__inner_list->size)

typedef struct __node_conf_data {
    uint8_t* configuration;
    double* alpha_decomp;
    double alpha;
} node_conf_data_t;


typedef struct __conf_list {
    list_t* __inner_list;
    double best_objective;
    size_t alpha_decomp_size;
    size_t atoms;
    size_t size;
} conf_list_t;

conf_list_t* conf_list_init(size_t atoms, size_t decomp_size);
bool conf_list_add(conf_list_t* l, double alpha, uint8_t* conf, double* decomp);
void conf_list_destroy(conf_list_t* l);
double conf_list_get_objective(conf_list_t* l, size_t index);
uint8_t* conf_list_get_conf(conf_list_t* l, size_t index);
double* conf_list_get_decomp(conf_list_t* l, size_t index);
