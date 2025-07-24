#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../include/libCacheSim/cache.h"
#include "../../include/libCacheSim/evictionAlgo.h"
#include "../../include/libCacheSim/macro.h"
#include "../../include/libCacheSim/const.h"
#include "../../include/libCacheSim/enum.h"
#include "../../include/libCacheSim/logging.h"
#include "../../include/libCacheSim/cacheObj.h"
#include "../../dataStructure/hashtable/hashtable.h"

#ifdef __cplusplus
extern "C" {
#endif

#define N_SAMPLE 16
#define N_TRAIN 1000
#define N_FEATURE 4
#define N_HIDDEN 32
#define N_OUTPUT 1
#define LEARNING_RATE 0.001

// Metadata for each cached object
typedef struct meta_t {
    obj_id_t obj_id;
    uint64_t size;
    uint64_t last_access;
    uint64_t freq;
    struct meta_t *prev, *next;
    // Add more fields as needed for ML features
} meta_t;

// Main 3L cache structure
typedef struct {
    uint64_t cache_size;
    uint64_t current_size;
    hashtable_t *table; // obj_id -> meta_t*
    meta_t *head, *tail; // LRU list
    // ML model placeholder (e.g., pointer to model struct)
    bool model_trained;
    // Add more fields as needed for sampling, stats, etc.
} cache_3l_t;

// Helper: remove a node from the LRU list
static void remove_from_list(cache_3l_t *cache, meta_t *node) {
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (cache->head == node) cache->head = node->next;
    if (cache->tail == node) cache->tail = node->prev;
    node->prev = node->next = NULL;
}

// Helper: insert node at head (MRU)
static void insert_at_head(cache_3l_t *cache, meta_t *node) {
    node->next = cache->head;
    node->prev = NULL;
    if (cache->head) cache->head->prev = node;
    cache->head = node;
    if (!cache->tail) cache->tail = node;
}

// lookup(): check if object is in cache, update metadata, move to head if hit
static bool cache_3l_lookup(cache_3l_t *cache, const request_t *req) {
    meta_t *meta = (meta_t *)hashtable_find_obj_id(cache->table, req->obj_id);
    if (meta) {
        // Cache hit: update metadata
        meta->last_access = req->clock_time;
        meta->freq++;
        remove_from_list(cache, meta);
        insert_at_head(cache, meta);
        // Optionally: update ML features here
        return true;
    }
    return false;
}

// admit(): insert new object into cache
static void cache_3l_admit(cache_3l_t *cache, const request_t *req) {
    // If object is too large, skip
    if (req->obj_size > cache->cache_size) return;
    // Create new meta
    meta_t *meta = (meta_t *)malloc(sizeof(meta_t));
    meta->obj_id = req->obj_id;
    meta->size = req->obj_size;
    meta->last_access = req->clock_time;
    meta->freq = 1;
    meta->prev = meta->next = NULL;
    // Insert into table and LRU list
    hashtable_insert(cache->table, req);
    insert_at_head(cache, meta);
    cache->current_size += req->obj_size;
    // Optionally: update ML features here
}

// evict(): evict objects until enough space, using LRU or ML-based policy
static void cache_3l_evict(cache_3l_t *cache) {
    // If model is not trained, use LRU
    while (cache->current_size > cache->cache_size && cache->tail) {
        meta_t *victim = cache->tail;
        // If model is trained, you can add ML-based candidate selection here
        // Placeholder: always evict LRU
        remove_from_list(cache, victim);
        hashtable_delete_obj_id(cache->table, victim->obj_id);
        cache->current_size -= victim->size;
        free(victim);
    }
}

// Main get() function: lookup, admit, evict as needed
static bool cache_3l_get(cache_t *cache, const request_t *req) {
    cache_3l_t *c = (cache_3l_t *)cache->eviction_params;
    if (cache_3l_lookup(c, req)) {
        return true;
    } else {
        // Miss: admit and evict as needed
        while (c->current_size + req->obj_size > c->cache_size) {
            cache_3l_evict(c);
        }
        cache_3l_admit(c, req);
        return false;
    }
}

// Free all resources
static void cache_3l_free(cache_t *cache) {
    cache_3l_t *c = (cache_3l_t *)cache->eviction_params;
    meta_t *cur = c->head;
    while (cur) {
        meta_t *next = cur->next;
        free(cur);
        cur = next;
    }
    free_hashtable(c->table);
    free(c);
    cache_struct_free(cache);
}

// Initialization
static cache_t *cache_3l_init(const common_cache_params_t ccache_params, const char *cache_specific_params) {
    cache_3l_t *c = (cache_3l_t *)malloc(sizeof(cache_3l_t));
    c->cache_size = ccache_params.cache_size;
    c->current_size = 0;
    c->head = c->tail = NULL;
    c->table = create_hashtable(ccache_params.hashpower);
    c->model_trained = false; // Placeholder for ML
    cache_t *cache = cache_struct_init("3L", ccache_params, cache_specific_params);
    cache->eviction_params = c;
    return cache;
}

cache_t *create_3l_cache(const common_cache_params_t ccache_params) {
    cache_t *cache = cache_struct_init("3L", ccache_params, NULL);
    cache->cache_init = cache_3l_init;
    cache->cache_free = cache_3l_free;
    cache->get = cache_3l_get;
    cache->evict = (void *)cache_3l_evict; // Not used directly, but for API compatibility
    return cache;
}

#ifdef __cplusplus
}
#endif 