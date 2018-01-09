#include <stdlib.h>
#include "conf_array.h"
#include "conf_list.h"

typedef struct __conf_collection_struct {
    double best_objective;
    size_t size;
    conf_list_t* __inner_list;
    conf_array_t* __inner_array;

} conf_collection_t;

conf_collection_t* conf_collection_init(size_t max_size, size_t atoms, size_t decomp_size);
uint8_t* conf_collection_get_conf(conf_collection_t* c, size_t index);
double* conf_collection_get_decomp(conf_collection_t* c, size_t index);
double conf_collection_get_objective(conf_collection_t* c, size_t index);
bool conf_collection_add(conf_collection_t* c, double objective, uint8_t *conf, double* decomp);
void conf_collection_destroy(conf_collection_t* c);
