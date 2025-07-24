// AdaptiveClimb.c - Original AdaptiveClimb eviction algorithm (no frequency)
#include "../../dataStructure/hashtable/hashtable.h"
#include "../../include/libCacheSim/evictionAlgo.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AdaptiveClimb_params {
    int jump;
    int K;
    cache_obj_t *q_head;
    cache_obj_t *q_tail;
} AdaptiveClimb_params_t;

#define HIT_MISS_WINDOW 1000
#define ADJUSTMENT_INTERVAL 1000
#define MIN_K 5
#define MAX_K 5000

static int recent_hits[HIT_MISS_WINDOW];
static int hit_miss_ptr = 0;
static int total_requests = 0;
static int recent_hit_count = 0;
static double last_miss_rate = 0.0;

static void update_hit_miss_window(int hit) {
    if (recent_hits[hit_miss_ptr]) recent_hit_count--;
    recent_hits[hit_miss_ptr] = hit;
    if (hit) recent_hit_count++;
    hit_miss_ptr = (hit_miss_ptr + 1) % HIT_MISS_WINDOW;
}

static void adjust_k_parameter(AdaptiveClimb_params_t *params) {
    if (total_requests % ADJUSTMENT_INTERVAL != 0) return;
    double miss_rate = 1.0 - ((double)recent_hit_count / HIT_MISS_WINDOW);
    if (miss_rate > last_miss_rate) {
        params->K = (params->K > MIN_K) ? params->K - 2 : MIN_K;
        params->jump = (params->jump < MAX_K) ? params->jump + 2 : MAX_K;
    } else {
        params->K = (params->K < MAX_K) ? params->K + 2 : MAX_K;
        params->jump = (params->jump > MIN_K) ? params->jump - 2 : MIN_K;
    }
    last_miss_rate = miss_rate;
}

static void AdaptiveClimb_free(cache_t *cache) {
    free(cache->eviction_params);
    cache_struct_free(cache);
}

static bool AdaptiveClimb_get(cache_t *cache, const request_t *req) {
    AdaptiveClimb_params_t *params = (AdaptiveClimb_params_t *)cache->eviction_params;
    cache_obj_t *obj = cache_find_base(cache, req, true);
    if (!obj) return false;
    total_requests++;
    update_hit_miss_window(1);
    adjust_k_parameter(params);
    // Remove from current position
    if (obj->queue.prev) {
        obj->queue.prev->queue.next = obj->queue.next;
    } else {
        params->q_head = obj->queue.next;
    }
    if (obj->queue.next) {
        obj->queue.next->queue.prev = obj->queue.prev;
    } else {
        params->q_tail = obj->queue.prev;
    }
    // Move to head (recency)
    obj->queue.next = params->q_head;
    obj->queue.prev = NULL;
    if (params->q_head) {
        params->q_head->queue.prev = obj;
    } else {
        params->q_tail = obj;
    }
    params->q_head = obj;
    return true;
}

static cache_obj_t *AdaptiveClimb_find(cache_t *cache, const request_t *req, const bool update_cache) {
    return cache_find_base(cache, req, update_cache);
}

static cache_obj_t *AdaptiveClimb_insert(cache_t *cache, const request_t *req) {
    AdaptiveClimb_params_t *params = (AdaptiveClimb_params_t *)cache->eviction_params;
    cache_obj_t *obj = cache_find_base(cache, req, false);
    if (obj) {
        AdaptiveClimb_get(cache, req);
        return obj;
    }
    total_requests++;
    update_hit_miss_window(0);
    adjust_k_parameter(params);
    // Evict if needed
    while (cache->get_occupied_byte(cache) + req->obj_size + cache->obj_md_size > cache->cache_size) {
        cache_obj_t *victim = params->q_tail;
        if (!victim) break;
        // Remove from queue
        if (victim->queue.prev) {
            victim->queue.prev->queue.next = NULL;
        } else {
            params->q_head = NULL;
        }
        params->q_tail = victim->queue.prev;
        cache_evict_base(cache, victim, true);
    }
    // Insert new object at head
    obj = cache_insert_base(cache, req);
    if (obj) {
        obj->queue.next = params->q_head;
        obj->queue.prev = NULL;
        if (params->q_head) {
            params->q_head->queue.prev = obj;
        } else {
            params->q_tail = obj;
        }
        params->q_head = obj;
    }
    return obj;
}

static cache_obj_t *AdaptiveClimb_to_evict(cache_t *cache, const request_t *req) {
    AdaptiveClimb_params_t *params = (AdaptiveClimb_params_t *)cache->eviction_params;
    return params->q_tail;
}

static void AdaptiveClimb_evict(cache_t *cache, const request_t *req) {
    AdaptiveClimb_params_t *params = (AdaptiveClimb_params_t *)cache->eviction_params;
    cache_obj_t *victim = params->q_tail;
    if (!victim) return;
    // Remove from queue
    if (victim->queue.prev) {
        victim->queue.prev->queue.next = NULL;
    } else {
        params->q_head = NULL;
    }
    params->q_tail = victim->queue.prev;
    cache_evict_base(cache, victim, true);
}

static bool AdaptiveClimb_remove(cache_t *cache, const obj_id_t obj_id) {
    cache_obj_t *obj = hashtable_find_obj_id(cache->hashtable, obj_id);
    if (!obj) return false;
    AdaptiveClimb_params_t *params = (AdaptiveClimb_params_t *)cache->eviction_params;
    // Remove from queue
    if (obj->queue.prev) {
        obj->queue.prev->queue.next = obj->queue.next;
    } else {
        params->q_head = obj->queue.next;
    }
    if (obj->queue.next) {
        obj->queue.next->queue.prev = obj->queue.prev;
    } else {
        params->q_tail = obj->queue.prev;
    }
    cache_remove_obj_base(cache, obj, true);
    return true;
}

cache_t *AdaptiveClimb_init(const common_cache_params_t ccache_params, const char *cache_specific_params) {
    cache_t *cache = cache_struct_init("AdaptiveClimb", ccache_params, cache_specific_params);
    cache->cache_init = AdaptiveClimb_init;
    cache->cache_free = AdaptiveClimb_free;
    cache->get = AdaptiveClimb_get;
    cache->find = AdaptiveClimb_find;
    cache->insert = AdaptiveClimb_insert;
    cache->evict = AdaptiveClimb_evict;
    cache->remove = AdaptiveClimb_remove;
    cache->to_evict = AdaptiveClimb_to_evict;
    AdaptiveClimb_params_t *params = malloc(sizeof(AdaptiveClimb_params_t));
    params->K = 10;
    params->jump = 1;
    params->q_head = NULL;
    params->q_tail = NULL;
    cache->eviction_params = params;
    return cache;
}

#ifdef __cplusplus
}
#endif 