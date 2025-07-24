// iLRU.c - Incremental LRU eviction algorithm
#include "../../dataStructure/hashtable/hashtable.h"
#include "../../include/libCacheSim/evictionAlgo.h"
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    cache_obj_t *q_head;
    cache_obj_t *q_tail;
} iLRU_params_t;

// Helper: get next caching state increment for an object
static size_t get_next_increment(cache_obj_t *obj) {
    // For demo: assume each object has a fixed increment of 1 (customize as needed)
    // In practice, you would use obj->caching_states and obj->current_state_idx
    return 1;
}

static void iLRU_free(cache_t *cache) { cache_struct_free(cache); }

static cache_obj_t *iLRU_find(cache_t *cache, const request_t *req, const bool update_cache) {
    iLRU_params_t *params = (iLRU_params_t *)cache->eviction_params;
    cache_obj_t *cache_obj = cache_find_base(cache, req, update_cache);
    if (cache_obj && likely(update_cache)) {
        move_obj_to_head(&params->q_head, &params->q_tail, cache_obj);
    }
    return cache_obj;
}

static cache_obj_t *iLRU_insert(cache_t *cache, const request_t *req) {
    iLRU_params_t *params = (iLRU_params_t *)cache->eviction_params;
    cache_obj_t *obj = cache_insert_base(cache, req);
    prepend_obj_to_head(&params->q_head, &params->q_tail, obj);
    return obj;
}

static cache_obj_t *iLRU_to_evict(cache_t *cache, const request_t *req) {
    iLRU_params_t *params = (iLRU_params_t *)cache->eviction_params;
    return params->q_tail;
}

static void iLRU_evict(cache_t *cache, const request_t *req) {
    iLRU_params_t *params = (iLRU_params_t *)cache->eviction_params;
    cache_obj_t *obj_to_evict = params->q_tail;
    assert(obj_to_evict != NULL);
    params->q_tail = params->q_tail->queue.prev;
    if (params->q_tail != NULL) {
        params->q_tail->queue.next = NULL;
    } else {
        params->q_head = NULL;
    }
    cache_evict_base(cache, obj_to_evict, true);
}

static bool iLRU_remove(cache_t *cache, const obj_id_t obj_id) {
    // Use a temporary request to look up the object by obj_id
    request_t req = {0};
    req.obj_id = obj_id;
    cache_obj_t *obj = cache_find_base(cache, &req, false);
    if (!obj) return false;
    iLRU_params_t *params = (iLRU_params_t *)cache->eviction_params;
    remove_obj_from_list(&params->q_head, &params->q_tail, obj);
    cache_remove_obj_base(cache, obj, true);
    return true;
}

static void iLRU_print_cache(const cache_t *cache) {
    // Optionally implement for debugging
}

// Incremental caching logic
static void iLRU_caching(cache_t *cache, cache_obj_t *obj) {
    // For demo: assume obj->cached_size and obj->full_size exist
    if (obj->cached_size < obj->full_size) {
        size_t d = get_next_increment(obj);
        // Evict until enough space
        while (cache->occupied_byte + d > cache->cache_size) {
            iLRU_evict(cache, NULL);
        }
        obj->cached_size += d;
        cache->occupied_byte += d;
        // Optionally update state index, etc.
    }
}

static bool iLRU_get(cache_t *cache, const request_t *req) {
    cache_obj_t *obj = iLRU_find(cache, req, true);
    if (obj) {
        iLRU_caching(cache, obj);
        return true;
    } else {
        obj = iLRU_insert(cache, req);
        iLRU_caching(cache, obj);
        return false;
    }
}

cache_t *iLRU_init(const common_cache_params_t ccache_params, const char *cache_specific_params) {
    cache_t *cache = cache_struct_init("iLRU", ccache_params, cache_specific_params);
    cache->cache_init = iLRU_init;
    cache->cache_free = iLRU_free;
    cache->get = iLRU_get;
    cache->find = iLRU_find;
    cache->insert = iLRU_insert;
    cache->evict = iLRU_evict;
    cache->remove = iLRU_remove;
    cache->to_evict = iLRU_to_evict;
    cache->get_occupied_byte = cache_get_occupied_byte_default;
    cache->can_insert = cache_can_insert_default;
    cache->get_n_obj = cache_get_n_obj_default;
    cache->print_cache = iLRU_print_cache;
    iLRU_params_t *params = malloc(sizeof(iLRU_params_t));
    params->q_head = NULL;
    params->q_tail = NULL;
    cache->eviction_params = params;
    return cache;
}

#ifdef __cplusplus
}
#endif 