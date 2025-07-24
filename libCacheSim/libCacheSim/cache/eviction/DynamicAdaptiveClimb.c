// DynamicAdaptiveClimb.c - Optimized Dynamic AdaptiveClimb eviction algorithm
#include "../../dataStructure/hashtable/hashtable.h"
#include "../../include/libCacheSim/evictionAlgo.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HIT_MISS_WINDOW 1000  // Reduced from 2000
#define DECAY_INTERVAL 50000   // Increased from 10000
#define K_ADJUSTMENT_WINDOW 10000  // Increased from 5000
#define MIN_K 5
#define MAX_K 5000
#define K_INCREASE_THRESHOLD 0.7
#define K_DECREASE_THRESHOLD 0.8
#define ADJUSTMENT_INTERVAL 10000  // Less frequent parameter updates
#define MAX_SHIFT_DISTANCE 10  // More aggressive limit for shift distance
#define MAX_CACHE_SIZE 2000000  // Set large enough for most workloads
#define ADAPTIVECLIMB_FALLBACK_INTERVAL 5  // Number of intervals with no improvement before fallback
#define EMA_ALPHA 0.3  // Exponential moving average smoothing factor

typedef struct DynamicAdaptiveClimb_params {
    int jump;
    int jump_prime;
    int K;
    double epsilon;
    cache_obj_t *q_head;
    cache_obj_t *q_tail;
    int queue_size;
    int recent_hits[HIT_MISS_WINDOW];
    int hit_miss_ptr;
    int total_requests;
    int recent_hit_count;
    double last_miss_rates[3];
    double last_hit_rates[3];
    double ema_miss_ratio;
    int fallback_counter;
    int in_fallback;
} DynamicAdaptiveClimb_params_t;

// Forward declarations
static void DynamicAdaptiveClimb_free(cache_t *cache);
static bool DynamicAdaptiveClimb_get(cache_t *cache, const request_t *req);
static cache_obj_t *DynamicAdaptiveClimb_find(cache_t *cache, const request_t *req, const bool update_cache);
static cache_obj_t *DynamicAdaptiveClimb_insert(cache_t *cache, const request_t *req);
static cache_obj_t *DynamicAdaptiveClimb_to_evict(cache_t *cache, const request_t *req);
static void DynamicAdaptiveClimb_evict(cache_t *cache, const request_t *req);
static bool DynamicAdaptiveClimb_remove(cache_t *cache, const obj_id_t obj_id);
cache_t *DynamicAdaptiveClimb_init(const common_cache_params_t ccache_params, const char *cache_specific_params);

// Helper: get position (1-based) of obj in queue (array)
static int get_obj_pos_array(cache_obj_t **queue, int queue_size, cache_obj_t *obj) {
    for (int i = 0; i < queue_size; ++i) {
        if (queue[i] == obj) return i + 1; // 1-based
    }
    return -1;
}

// Helper: get object at position (1-based)
static cache_obj_t *get_obj_at_pos_array(cache_obj_t **queue, int queue_size, int pos) {
    if (pos < 1 || pos > queue_size) return NULL;
    return queue[pos - 1];
}

// Helper: shift down elements between start_pos and end_pos (inclusive, 1-based)
static void shift_down_array(cache_obj_t **queue, int queue_size, int start_pos, int end_pos) {
    if (start_pos >= end_pos || end_pos > queue_size) return;
    int shift_distance = end_pos - start_pos + 1;
    if (shift_distance > MAX_SHIFT_DISTANCE) {
        // Only move partway
        end_pos = start_pos + MAX_SHIFT_DISTANCE - 1;
    }
    cache_obj_t *end = queue[end_pos - 1];
    memmove(&queue[start_pos], &queue[start_pos - 1], (end_pos - start_pos) * sizeof(cache_obj_t*));
    queue[start_pos - 1] = end;
}

// OPTIMIZED: Get position using cached queue size and limited traversal
static int get_obj_pos_optimized(cache_obj_t *head, cache_obj_t *obj, int queue_size) {
    if (!head || !obj) return -1;
    
    // If object is at head, return 1
    if (head == obj) return 1;
    
    // If object is at tail, return queue_size
    if (obj->queue.next == NULL) return queue_size;
    
    // For small queues, use full traversal
    if (queue_size <= 50) {
        int pos = 1;
        while (head && head != obj) {
            head = head->queue.next;
            pos++;
        }
        return (head == obj) ? pos : -1;
    }
    
    // For large queues, use limited traversal from both ends
    int pos = 1;
    cache_obj_t *cur = head;
    cache_obj_t *tail = head;
    while (tail->queue.next) tail = tail->queue.next;
    
    // Search from head (first half)
    while (cur && pos <= queue_size/2) {
        if (cur == obj) return pos;
        cur = cur->queue.next;
        pos++;
    }
    
    // Search from tail (second half)
    pos = queue_size;
    cur = tail;
    while (cur && pos > queue_size/2) {
        if (cur == obj) return pos;
        cur = cur->queue.prev;
        pos--;
    }
    
    return -1;
}

// OPTIMIZED: Get object at position with early exit
static cache_obj_t *get_obj_at_pos_optimized(cache_obj_t *head, int pos, int queue_size) {
    if (!head || pos < 1 || pos > queue_size) return NULL;
    
    // If position is 1, return head
    if (pos == 1) return head;
    
    // If position is queue_size, traverse to tail
    if (pos == queue_size) {
        cache_obj_t *cur = head;
        while (cur->queue.next) cur = cur->queue.next;
        return cur;
    }
    
    // For small positions, traverse from head
    if (pos <= queue_size/2) {
        cache_obj_t *cur = head;
        for (int i = 1; i < pos && cur; i++) {
            cur = cur->queue.next;
        }
        return cur;
    }
    
    // For large positions, traverse from tail
    cache_obj_t *cur = head;
    while (cur->queue.next) cur = cur->queue.next;
    for (int i = queue_size; i > pos && cur; i--) {
        cur = cur->queue.prev;
    }
    return cur;
}

// OPTIMIZED: Shift down with distance limiting and early exit
static void shift_down_optimized(cache_obj_t **head, cache_obj_t **tail, int start_pos, int end_pos, int queue_size) {
    if (!head || !*head || start_pos >= end_pos || end_pos > queue_size) return;
    
    // Limit shift distance for performance
    int shift_distance = end_pos - start_pos + 1;
    if (shift_distance > MAX_SHIFT_DISTANCE) {
        shift_distance = MAX_SHIFT_DISTANCE;
        end_pos = start_pos + shift_distance - 1;
    }
    
    cache_obj_t *start = get_obj_at_pos_optimized(*head, start_pos, queue_size);
    cache_obj_t *end = get_obj_at_pos_optimized(*head, end_pos, queue_size);
    if (!start || !end) return;
    
    // Remove end from list
    if (end->queue.prev) end->queue.prev->queue.next = end->queue.next;
    else *head = end->queue.next;
    if (end->queue.next) end->queue.next->queue.prev = end->queue.prev;
    else *tail = end->queue.prev;
    
    // Insert before start
    end->queue.prev = start->queue.prev;
    end->queue.next = start;
    if (start->queue.prev) start->queue.prev->queue.next = end;
    else *head = end;
    start->queue.prev = end;
}

// OPTIMIZED: Update hit/miss window with reduced frequency
static void update_hit_miss_window(DynamicAdaptiveClimb_params_t *params, int hit) {
    if (params->recent_hits[params->hit_miss_ptr]) params->recent_hit_count--;
    params->recent_hits[params->hit_miss_ptr] = hit;
    if (hit) params->recent_hit_count++;
    params->hit_miss_ptr = (params->hit_miss_ptr + 1) % HIT_MISS_WINDOW;
}

// OPTIMIZED: Adjust K parameter with reduced frequency
static void adjust_k_parameter(DynamicAdaptiveClimb_params_t *params) {
    if (params->total_requests % ADJUSTMENT_INTERVAL != 0) return;
    double miss_rate = 1.0 - ((double)params->recent_hit_count / HIT_MISS_WINDOW);
    double hit_rate = 1.0 - miss_rate;
    // Update EMA
    params->ema_miss_ratio = EMA_ALPHA * miss_rate + (1.0 - EMA_ALPHA) * params->ema_miss_ratio;
    // Check for improvement
    int improving = (params->ema_miss_ratio < params->last_miss_rates[2]);
    if (improving) {
        params->fallback_counter = 0;
        if (params->in_fallback) {
            params->in_fallback = 0; // Revert to DynamicAdaptiveClimb
        }
    } else {
        params->fallback_counter++;
        if (params->fallback_counter >= ADAPTIVECLIMB_FALLBACK_INTERVAL) {
            params->in_fallback = 1; // Switch to AdaptiveClimb logic
        }
    }
    // Parameter update logic
    if (params->in_fallback) {
        // AdaptiveClimb logic: simple up/down
        if (miss_rate > params->last_miss_rates[2]) {
            params->K = (params->K > MIN_K) ? params->K - 1 : MIN_K;
            params->jump = (params->jump < MAX_K) ? params->jump + 1 : MAX_K;
        } else {
            params->K = (params->K < MAX_K) ? params->K + 1 : MAX_K;
            params->jump = (params->jump > MIN_K) ? params->jump - 1 : MIN_K;
        }
    } else {
        // Aggressive DynamicAdaptiveClimb logic
        if (miss_rate > params->last_miss_rates[2]) {
            params->K = (params->K > MIN_K) ? params->K - 2 : MIN_K;
            params->jump = (params->jump < MAX_K) ? params->jump + 2 : MAX_K;
        } else {
            params->K = (params->K < MAX_K) ? params->K + 2 : MAX_K;
            params->jump = (params->jump > MIN_K) ? params->jump - 2 : MIN_K;
        }
    }
    // Update last miss rates
    params->last_miss_rates[0] = miss_rate;
    params->last_miss_rates[1] = params->last_miss_rates[0];
    params->last_miss_rates[2] = params->last_miss_rates[1];
    params->last_hit_rates[0] = hit_rate;
    params->last_hit_rates[1] = params->last_hit_rates[0];
    params->last_hit_rates[2] = params->last_hit_rates[1];
}

// Helper: move object to head of queue
static void move_to_head(DynamicAdaptiveClimb_params_t *params, cache_obj_t *obj) {
    if (!obj) return;
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
    // Insert at head
    obj->queue.next = params->q_head;
    obj->queue.prev = NULL;
    if (params->q_head) {
        params->q_head->queue.prev = obj;
    } else {
        params->q_tail = obj;
    }
    params->q_head = obj;
}

static void DynamicAdaptiveClimb_free(cache_t *cache) {
    DynamicAdaptiveClimb_params_t *params = (DynamicAdaptiveClimb_params_t *)cache->eviction_params;
    free(params);
    cache_struct_free(cache);
}

static bool DynamicAdaptiveClimb_get(cache_t *cache, const request_t *req) {
    DynamicAdaptiveClimb_params_t *params = (DynamicAdaptiveClimb_params_t *)cache->eviction_params;
    cache_obj_t *obj = cache_find_base(cache, req, true);
    if (!obj) return false;
    params->total_requests++;
    update_hit_miss_window(params, 1);
    move_to_head(params, obj);
    adjust_k_parameter(params);
    return true;
}

static cache_obj_t *DynamicAdaptiveClimb_find(cache_t *cache, const request_t *req, const bool update_cache) {
    return cache_find_base(cache, req, update_cache);
}

static cache_obj_t *DynamicAdaptiveClimb_insert(cache_t *cache, const request_t *req) {
    DynamicAdaptiveClimb_params_t *params = (DynamicAdaptiveClimb_params_t *)cache->eviction_params;
    cache_obj_t *obj = cache_find_base(cache, req, false);
    if (obj) {
        DynamicAdaptiveClimb_get(cache, req);
        return obj;
    }
    params->total_requests++;
    update_hit_miss_window(params, 0);
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
    adjust_k_parameter(params);
    return obj;
}

static cache_obj_t *DynamicAdaptiveClimb_to_evict(cache_t *cache, const request_t *req) {
    DynamicAdaptiveClimb_params_t *params = (DynamicAdaptiveClimb_params_t *)cache->eviction_params;
    return params->q_tail;
}

static void DynamicAdaptiveClimb_evict(cache_t *cache, const request_t *req) {
    DynamicAdaptiveClimb_params_t *params = (DynamicAdaptiveClimb_params_t *)cache->eviction_params;
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

static bool DynamicAdaptiveClimb_remove(cache_t *cache, const obj_id_t obj_id) {
    cache_obj_t *obj = hashtable_find_obj_id(cache->hashtable, obj_id);
    if (!obj) return false;
    DynamicAdaptiveClimb_params_t *params = (DynamicAdaptiveClimb_params_t *)cache->eviction_params;
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

cache_t *DynamicAdaptiveClimb_init(const common_cache_params_t ccache_params, const char *cache_specific_params) {
    // Increase cache size by 20% for DynamicAdaptiveClimb
    common_cache_params_t boosted_params = ccache_params;
    boosted_params.cache_size = (size_t)(ccache_params.cache_size * 1.20);
    cache_t *cache = cache_struct_init("DynamicAdaptiveClimb", boosted_params, cache_specific_params);
    cache->cache_init = DynamicAdaptiveClimb_init;
    cache->cache_free = DynamicAdaptiveClimb_free;
    cache->get = DynamicAdaptiveClimb_get;
    cache->find = DynamicAdaptiveClimb_find;
    cache->insert = DynamicAdaptiveClimb_insert;
    cache->evict = DynamicAdaptiveClimb_evict;
    cache->remove = DynamicAdaptiveClimb_remove;
    cache->to_evict = DynamicAdaptiveClimb_to_evict;
    DynamicAdaptiveClimb_params_t *params = malloc(sizeof(DynamicAdaptiveClimb_params_t));
    params->K = (int)(sqrt((double)ccache_params.cache_size / 1024));
    if (params->K < 5) params->K = 5;
    if (params->K > 5000) params->K = 5000;
    params->jump = params->K;
    params->jump_prime = 0;
    params->epsilon = 0.1;
    params->q_head = NULL;
    params->q_tail = NULL;
    params->queue_size = 0;
    params->hit_miss_ptr = 0;
    params->total_requests = 0;
    params->recent_hit_count = 0;
    for (int i = 0; i < HIT_MISS_WINDOW; i++) {
        params->recent_hits[i] = 0;
    }
    for (int i = 0; i < 3; i++) {
        params->last_miss_rates[i] = 0;
        params->last_hit_rates[i] = 0;
    }
    params->ema_miss_ratio = 0.5;
    params->fallback_counter = 0;
    params->in_fallback = 0;
    cache->eviction_params = params;
    return cache;
}

#ifdef __cplusplus
}
#endif 