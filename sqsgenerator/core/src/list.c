//
//  list.c
//  list
//
//  Created by Dominik on 25.06.17.
//  Copyright © 2017 Dominik. All rights reserved.
//


#include <stdio.h>
#include "list.h"

/* Locks the lists mutex only if the lock was not acquired */
void list_acquire_mutex(list_t* l){
    if (!l->lock_aquired) {
        pthread_mutex_lock(&(l->mutex));
        l->lock_aquired = true;
    }
}
/* Releases the lists mutex only if the lock was acquired */
void list_release_mutex(list_t* l){
    if (l->lock_aquired) {
        l->lock_aquired = false;
        pthread_mutex_unlock(&(l->mutex));
    }
}

list_t* list_init(void (*destroy)(void *data, void *meta_data)){
    list_t *ll = malloc(sizeof(list_t));
    if (!ll) {
        return NULL;
    }
    ll->size = 0;
    ll->head = NULL;
    ll->tail = NULL;
    ll->lock_aquired = false;
    ll->destroy = destroy;
    pthread_mutex_init(&(ll->mutex), NULL);
    return ll;
}

bool list_append(list_t* l, void* data, void* meta_data){
    node_t *element = malloc(sizeof(node_t));
    if (!element) {
        return false;
    }
    list_acquire_mutex(l);
    element->data = data;
    element->meta_data = meta_data;
    if (l->size == 0) {
        l->head = element;
        l->tail = element;
        element->before = NULL;
        element->next = NULL;
        l->size = 1;
    }
    else
    {
        l->tail->next = element;
        element->next = NULL;
        element->before = l->tail;
        l->tail = element;
        l->size++;
    }
    list_release_mutex(l);
    return true;
}

bool list_push(list_t* l, void* data, void* meta_data){
    node_t *element = malloc(sizeof(node_t));
    if (!element) {
        return false;
    }
    list_acquire_mutex(l);
    element->data = data;
    element->meta_data = meta_data;
    if (l->size == 0) {
        return list_append(l, data, meta_data);
    }
    else
    {
        element->next = l->head;
        element->before = NULL;
        l->head->before = element;
        l->head = element;
        l->size++;
    }
    list_release_mutex(l);
    return true;
}

node_t *list_get_node(list_t* l, size_t index){
    list_acquire_mutex(l);
    if (index < l->size) {
        node_t *n;
        size_t i;
        if (index <= l->size/2) {

            n = l->head;
            for (i = 0; i < index; i++) {
                n = n->next;
            }
            list_release_mutex(l);
            return n;
        }
        else {
            n = l->tail;
            for (i = (l->size - 1); i > index; i--) {
                n = n->before;
            }
            list_release_mutex(l);
            return n;
        }
    }
    list_release_mutex(l);
    return NULL;
}

void* list_get_data(list_t* l, size_t index){
    node_t *n = list_get_node(l, index);
    if (!n) {
        return NULL;
    }
    return n->data;
}

void* list_get_meta_data(list_t* l, size_t index){
    node_t *n = list_get_node(l, index);
    if (!n) {
        return NULL;
    }
    return n->meta_data;
}

bool list_insert(list_t* l, void* data, void* meta_data, size_t index) {
    if (index < l->size) {
        if (index == l->size-1) {
            return list_append(l, data, meta_data);
        }
        if (index == 0) {
            return list_push(l, data, meta_data);
        }
        node_t *n = list_get_node(l, index);
        node_t *element = malloc(sizeof(node_t));
        if (!element) {
            return false;
        }
        list_acquire_mutex(l);
        element->next = n;
        element->before = n->before;
        n->before->next = element;
        n->before =element;
        l->size++;
        list_release_mutex(l);
        return true;
    }
    return false;
}

void list_destroy(list_t* l) {
    if (l) {
        list_clear(l);
        pthread_mutex_destroy(&(l->mutex));
        free(l);
    }
}

void list_clear(list_t* l){
    list_acquire_mutex(l);
    node_t *n = l->head;
    node_t *tmp;
    while (n) {
        if (l->destroy) {
            l->destroy(n->data, n->meta_data);
        }

        tmp = n->next;
        free(n);
        n = tmp;
    }
    l->size = 0;
    l->head = NULL;
    l->tail = NULL;
    list_release_mutex(l);
}

node_t* list_remove_node(list_t* l, size_t index) {
    node_t *n;
    if (index == 0) {
        list_acquire_mutex(l);
        n = l->head;
        if (n) {
            n->next->before = NULL;
            l->head = n->next;
            n->next = NULL;
            list_release_mutex(l);
            return n;
        }
    }
    if(index == l->size-1){
        list_acquire_mutex(l);
        n = l->tail;
        if (n) {
            n->before->next = NULL;
            l->tail = n->before;
            n->before = NULL;
            list_release_mutex(l);
            return n->data;
        }
    }
    else {
        list_acquire_mutex(l);
        n = list_get_node(l, index);
        n->before->next = n->next;
        n->next->before = n->before;
        n->next = NULL;
        n->before = NULL;
        list_release_mutex(l);
        return n;
    }
    return NULL;
}

void* list_pop(list_t* l){
    node_t* removed = list_remove_node(l, l->size-1);
    return removed->data;
}
