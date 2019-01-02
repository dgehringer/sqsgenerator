#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct __node_struct {
    void* data;
    void* meta_data;
    struct __node_struct* before;
    struct __node_struct* next;
} node_t;

typedef struct __list_struct {
    struct __node_struct* head;
    struct __node_struct* tail;
    void (*destroy)(void *data, void *meta_data);
    pthread_mutex_t mutex;
    size_t size;
}list_t;

list_t* list_init(void (*destroy)(void *data, void *meta_data));
bool list_append(list_t* l, void* data, void* meta_data);
bool __list_append_internal(list_t* l, void* data, void* meta_data);
bool list_push(list_t* l, void* data, void* meta_data);
bool __list_push_internal(list_t* l, void* data, void* meta_data);
node_t *list_get_node(list_t* l, size_t index);
node_t *__list_get_node_internal(list_t* l, size_t index);
void* list_get_data(list_t* l, size_t index);
void* list_get_meta_data(list_t* l, size_t index);
bool list_insert(list_t* l, void* data, void* meta_data, size_t index);
bool __list_insert_internal(list_t* l, void* data, void* meta_data, size_t index);
void list_destroy(list_t* l);
node_t* list_remove_node(list_t* l, size_t index);
node_t* __list_remove_node_internal(list_t* l, size_t index);
void* list_pop(list_t* l);
void* __list_pop_internal(list_t* l);
void list_clear(list_t* l);
void __list_clear_internal(list_t* l);
void list_acquire_mutex(list_t* l);
void list_release_mutex(list_t* l);
