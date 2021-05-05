// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind_memtier.h>

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_log.h>

#include "config.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#define MEMKIND_ATOMIC _Atomic
#else
#define MEMKIND_ATOMIC
#endif

// clang-format off
#if defined(MEMKIND_ATOMIC_C11_SUPPORT)
#define memkind_atomic_increment(counter, val)                                 \
    atomic_fetch_add_explicit(&counter, val, memory_order_relaxed)
#define memkind_atomic_decrement(counter, val)                                 \
    atomic_fetch_sub_explicit(&counter, val, memory_order_relaxed)
#define memkind_atomic_set(counter, val)                                       \
    atomic_store_explicit(&counter, val, memory_order_relaxed)
#define memkind_atomic_get(src, dest)                                          \
    do {                                                                       \
        dest = atomic_load_explicit(&src, memory_order_relaxed);               \
    } while (0)
#define memkind_atomic_get_and_zeroing(src)                                    \
    atomic_exchange_explicit(&src, 0, memory_order_relaxed)
#elif defined(MEMKIND_ATOMIC_BUILTINS_SUPPORT)
#define memkind_atomic_increment(counter, val)                                 \
    __atomic_fetch_add(&counter, val, __ATOMIC_RELAXED)
#define memkind_atomic_decrement(counter, val)                                 \
    __atomic_fetch_sub(&counter, val, __ATOMIC_RELAXED)
#define memkind_atomic_set(counter, val)                                       \
    __atomic_store_n(&counter, val, __ATOMIC_RELAXED)
#define memkind_atomic_get(src, dest)                                          \
    do {                                                                       \
        dest = __atomic_load_n(&src, __ATOMIC_RELAXED);                        \
    } while (0)
#define memkind_atomic_get_and_zeroing(src)                                    \
    __atomic_exchange_n(&src, 0, __ATOMIC_RELAXED)
#elif defined(MEMKIND_ATOMIC_SYNC_SUPPORT)
#define memkind_atomic_increment(counter, val)                                 \
    __sync_fetch_and_add(&counter, val)
#define memkind_atomic_decrement(counter, val)                                 \
    __sync_fetch_and_sub(&counter, val)
#define memkind_atomic_get(src, dest)                                          \
    do {                                                                       \
        dest = __sync_sub_and_fetch(&src, 0);                                  \
    } while (0)
#define memkind_atomic_get_and_zeroing(src)                                    \
    do {                                                                       \
        dest = __sync_fetch_and_sub(&src, &src);                               \
    } while (0)
#define memkind_atomic_set(counter, val)                                       \
    __atomic_store_n(&counter, val, __ATOMIC_RELAXED)
#else
#error "Missing atomic implementation."
#endif

// Default values for DYNAMIC_THRESHOLD configuration
// TRIGGER       - threshold between tiers will be updated if a difference
//                 between current and desired ratio between these tiers is
//                 greater than TRIGGER value (in percents)
// DEGREE        - if an update is triggered, DEGREE is the value (in percents)
//                 by which threshold will change
// CHECK_CNT     - number of memory management operations that has to be made
//                 between ratio checks
// STEP          - default step (in bytes) between thresholds
#define THRESHOLD_TRIGGER   0.1  // 10%
#define THRESHOLD_DEGREE    0.25 // 25%
#define THRESHOLD_CHECK_CNT 5
#define THRESHOLD_STEP      1024

// Macro to get number of thresholds from parent object
#define THRESHOLD_NUM(obj) ((obj->cfg_size) - 1)

struct memtier_tier_cfg {
    memkind_t kind;   // Memory kind
    float kind_ratio; // Memory kind ratio
};

// Thresholds configuration - valid only for DYNAMIC_THRESHOLD policy
struct memtier_threshold_cfg {
    size_t val;       // Actual threshold level
    size_t min;       // Minimum threshold level
    size_t max;       // Maximum threshold level
    float norm_ratio; // Normalized ratio between two adjacent tiers
};

struct memtier_builder {
    memtier_policy_t policy;      // Tiering policy
    unsigned cfg_size;            // Number of Memory Tier configurations
    struct memtier_tier_cfg *cfg; // Memory Tier configurations
    unsigned thres_size;          // Number of Memory threshold configurations
    struct memtier_threshold_cfg *thres; // Thresholds configuration for
                                         // DYNAMIC_THRESHOLD policy
    unsigned check_cnt; // Number of memory management operations that has to be
                        // made between ratio checks
    float trigger;      // Difference between ratios to update threshold
    float degree;       // % of threshold change in case of update
    size_t step;        // Step (in bytes) between thresholds
    // builder operations
    int (*ctl_set)(struct memtier_builder *builder, const char *name, const void *val);
};

struct memtier_memory {
    unsigned cfg_size;                   // Number of memory kinds
    struct memtier_tier_cfg *cfg;        // Memory Tier configuration
    struct memtier_threshold_cfg *thres; // Thresholds configuration for
                                         // DYNAMIC_THRESHOLD policy
    unsigned thres_check_cnt;            // Counter for doing thresholds check
    unsigned thres_init_check_cnt;       // Initial value of check_cnt
    float thres_trigger;                 // Difference between ratios to update
                                         // threshold
    float thres_degree; // % of threshold change in case of update

    memkind_t (*get_kind)(struct memtier_memory *memory, size_t size);
    void (*update_cfg)(struct memtier_memory *memory);
};
// clang-format on

#define THREAD_BUCKETS  (256U)
#define FLUSH_THRESHOLD (51200)

static MEMKIND_ATOMIC long long t_alloc_size[MEMKIND_MAX_KIND][THREAD_BUCKETS];
static MEMKIND_ATOMIC size_t g_alloc_size[MEMKIND_MAX_KIND];

void memtier_reset_size(unsigned kind_id)
{
    unsigned bucket_id;
    for (bucket_id = 0; bucket_id < THREAD_BUCKETS; ++bucket_id) {
        memkind_atomic_set(t_alloc_size[kind_id][bucket_id], 0);
    }
    memkind_atomic_set(g_alloc_size[kind_id], 0);
}

// SplitMix64 hash
static inline unsigned t_hash_64(void)
{
    uint64_t x = (uint64_t)pthread_self();
    x += 0x9e3779b97f4a7c15;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    return (x ^ (x >> 31)) & (THREAD_BUCKETS - 1);
}

static inline void increment_alloc_size(unsigned kind_id, size_t size)
{
    unsigned bucket_id = t_hash_64();
    if ((memkind_atomic_increment(t_alloc_size[kind_id][bucket_id], size) +
         size) > FLUSH_THRESHOLD) {
        size_t size_f =
            memkind_atomic_get_and_zeroing(t_alloc_size[kind_id][bucket_id]);
        memkind_atomic_increment(g_alloc_size[kind_id], size_f);
    }
}

static inline void decrement_alloc_size(unsigned kind_id, size_t size)
{
    unsigned bucket_id = t_hash_64();
    if ((memkind_atomic_decrement(t_alloc_size[kind_id][bucket_id], size) -
         size) < -FLUSH_THRESHOLD) {
        long long size_f =
            memkind_atomic_get_and_zeroing(t_alloc_size[kind_id][bucket_id]);
        memkind_atomic_increment(g_alloc_size[kind_id], size_f);
    }
}

static memkind_t memtier_single_get_kind(struct memtier_memory *memory,
                                         size_t size)
{
    return memory->cfg[0].kind;
}

static memkind_t
memtier_policy_static_threshold_get_kind(struct memtier_memory *memory,
                                         size_t size)
{
    struct memtier_tier_cfg *cfg = memory->cfg;

    size_t size_tier, size_0;
    int i;
    int dest_tier = 0;
    memkind_atomic_get(g_alloc_size[cfg[0].kind->partition], size_0);
    for (i = 1; i < memory->cfg_size; ++i) {
        memkind_atomic_get(g_alloc_size[cfg[i].kind->partition], size_tier);
        if ((size_tier * cfg[i].kind_ratio) < size_0) {
            dest_tier = i;
        }
    }
    return cfg[dest_tier].kind;
}

static memkind_t
memtier_policy_dynamic_threshold_get_kind(struct memtier_memory *memory,
                                          size_t size)
{
    struct memtier_threshold_cfg *thres = memory->thres;
    int i;

    for (i = 0; i < THRESHOLD_NUM(memory); ++i) {
        if (size < thres[i].val) {
            break;
        }
    }
    return memory->cfg[i].kind;
}

static void print_memtier_memory(struct memtier_memory *memory)
{
    int i;
    if (!memory) {
        log_info("Empty memtier memory");
        return;
    }
    log_info("Number of memory tiers %u", memory->cfg_size);
    for (i = 0; i < memory->cfg_size; ++i) {
        log_info("Tier %d - memory kind %s", i, memory->cfg[i].kind->name);
        log_info("Tier normalized ratio %f", memory->cfg[i].kind_ratio);
        log_info("Tier allocated size %zu",
                 memtier_kind_allocated_size(memory->cfg[i].kind));
    }
    if (memory->thres) {
        for (i = 0; i < THRESHOLD_NUM(memory); ++i) {
            log_info("Threshold %d - minimum %zu", i, memory->thres[i].min);
            log_info("Threshold %d - current value %zu", i,
                     memory->thres[i].val);
            log_info("Threshold %d - maximum %zu", i, memory->thres[i].max);
        }
    } else {
        log_info("No thresholds configuration found");
    }
    log_info("Threshold trigger value %f", memory->thres_trigger);
    log_info("Threshold degree value %f", memory->thres_degree);
    log_info("Threshold counter setting value %u",
             memory->thres_init_check_cnt);
    log_info("Threshold counter current value %u", memory->thres_check_cnt);
}

static void print_builder(struct memtier_builder *builder)
{
    int i;
    if (!builder) {
        log_info("Empty builder");
        return;
    }
    log_info("Number of memory tiers %u", builder->cfg_size);
    for (i = 0; i < builder->cfg_size; ++i) {
        log_info("Tier %d - memory kind %s", i, builder->cfg[i].kind->name);
        log_info("Tier normalized ratio %f", builder->cfg[i].kind_ratio);
    }
    if (builder->thres) {
        for (i = 0; i < THRESHOLD_NUM(builder); ++i) {
            log_info("Threshold %d - minimum %zu", i, builder->thres[i].min);
            log_info("Threshold %d - current value %zu", i,
                     builder->thres[i].val);
            log_info("Threshold %d - maximum %zu", i, builder->thres[i].max);
        }
    } else {
        log_info("No thresholds configuration found");
    }
    log_info("Threshold trigger value %f", builder->trigger);
    log_info("Threshold degree value %f", builder->degree);
    log_info("Threshold counter setting value %u", builder->check_cnt);
}

static void
memtier_policy_static_threshold_update_config(struct memtier_memory *memory)
{}

static void
memtier_policy_dynamic_threshold_update_config(struct memtier_memory *memory)
{
    struct memtier_tier_cfg *cfg = memory->cfg;
    struct memtier_threshold_cfg *thres = memory->thres;
    int i;
    size_t prev_alloc_size, next_alloc_size;

    // do the ratio checks only every each thres_check_cnt
    if (--memory->thres_check_cnt > 0) {
        return;
    }

    // for every pair of adjacent tiers, check if distance between actual vs
    // desired ratio between them is above TRIGGER level and if so, change
    // threshold by CHANGE val
    // TODO optimize the loop to avoid redundant atomic_get in 3 or more tier
    // scenario
    for (i = 0; i < THRESHOLD_NUM(memory); ++i) {
        memkind_atomic_get(g_alloc_size[cfg[i].kind->partition],
                           prev_alloc_size);
        memkind_atomic_get(g_alloc_size[cfg[i + 1].kind->partition],
                           next_alloc_size);

        float current_ratio = -1;
        if (prev_alloc_size > 0) {
            current_ratio = (float)next_alloc_size / prev_alloc_size;
            float ratio_diff = current_ratio - thres[i].norm_ratio;
            if (fabs(ratio_diff) < memory->thres_trigger) {
                // threshold needn't to be changed
                continue;
            }
        }

        // increase/decrease threshold value by thres_degree and clamp it to
        // (min, max) range
        size_t threshold = thres[i].val * memory->thres_degree;
        if ((prev_alloc_size == 0) || (current_ratio > thres[i].norm_ratio)) {
            size_t higher_threshold = thres[i].val + threshold;
            if (higher_threshold <= thres[i].max) {
                thres[i].val = higher_threshold;
            }
        } else {
            size_t lower_threshold = thres[i].val - threshold;
            if (lower_threshold >= thres[i].min) {
                thres[i].val = lower_threshold;
            }
        }
    }

    // reset threshold check counter
    memory->thres_check_cnt = memory->thres_init_check_cnt;
}

static int builder_static_ctl_set(struct memtier_builder *builder,
                                  const char *name, const void *val)
{
    log_err("Invalid name: %s", name);
    return -1;
}

static int memtier_builder_create_threshold(struct memtier_builder *builder,
                                            unsigned id)
{
    if (builder->thres_size > id) {
        // nothing to do
        return 0;
    }

    struct memtier_threshold_cfg *thres =
        jemk_realloc(builder->thres, sizeof(*thres) * (id + 1));

    if (!thres) {
        log_err("realloc() failed.");
        return -1;
    }

    int old_size = builder->thres_size;
    builder->thres_size = id + 1;
    builder->thres = thres;
    int i;
    for (i = old_size; i < builder->thres_size; ++i) {
        if (i == 0) {
            builder->thres[i].val = builder->step;
            builder->thres[i].min = (float)builder->step * 0.5;
            builder->thres[i].max = (float)builder->step * 1.5 - 1;
        } else {
            builder->thres[i].val =
                builder->thres[i - i].max + (float)builder->step * 0.5;
            builder->thres[i].min = builder->thres[i - 1].max + 1;
            builder->thres[i].max =
                builder->thres[i].val + (float)builder->step * 0.5 - 1;
        }
        // NOTE: don't set norm_ratio here - it will be calculated during
        // memtier memory construction
    }

    return 0;
}

static int builder_dynamic_ctl_set(struct memtier_builder *builder,
                                   const char *name, const void *val)
{
    const char *query = name;
    char name_substr[256] = {0};
    int chr_read = 0;

    sscanf(query, "policy.dynamic_threshold.%n", &chr_read);
    if (chr_read == sizeof("policy.dynamic_threshold.") - 1) {
        query += chr_read;

        int ret = sscanf(query, "%[^\[]%n", name_substr, &chr_read);
        if (ret && strcmp(name_substr, "thresholds") == 0) {
            query += chr_read;

            int th_id = -1;
            ret = sscanf(query, "[%d]%n", &th_id, &chr_read);
            if (th_id >= 0) {
                query += chr_read;

                // make sure that threshold configuration of th_id exist
                int ret = memtier_builder_create_threshold(builder, th_id);
                if (ret != 0) {
                    return -1;
                }

                struct memtier_threshold_cfg *thres = &builder->thres[th_id];

                ret = sscanf(query, ".%s", name_substr);
                if (ret && strcmp(name_substr, "val") == 0) {
                    thres->val = *(size_t *)val;
                    return 0;
                } else if (ret && strcmp(name_substr, "min") == 0) {
                    thres->min = *(size_t *)val;
                    return 0;
                } else if (ret && strcmp(name_substr, "max") == 0) {
                    thres->max = *(size_t *)val;
                    return 0;
                }
            }
        } else if (ret && strcmp(name_substr, "check_cnt") == 0) {
            builder->check_cnt = *(unsigned *)val;
            return 0;
        } else if (ret && strcmp(name_substr, "trigger") == 0) {
            builder->trigger = *(float *)val;
            return 0;
        } else if (ret && strcmp(name_substr, "degree") == 0) {
            builder->degree = *(float *)val;
            return 0;
        } else if (ret && strcmp(name_substr, "step") == 0) {
            builder->step = *(size_t *)val;
            return 0;
        }
    }

    log_err("Invalid name: %s", query);
    return -1;
}

// clang-format off
MEMKIND_EXPORT struct memtier_builder *memtier_builder_new(memtier_policy_t policy)
{
    struct memtier_builder *b = jemk_calloc(1, sizeof(struct memtier_builder));
    if (b) {
        switch (policy) {
            case MEMTIER_POLICY_STATIC_THRESHOLD:
                b->policy = policy;
                b->ctl_set = builder_static_ctl_set;
                return b;
            case MEMTIER_POLICY_DYNAMIC_THRESHOLD:
                b->policy = policy;
                b->check_cnt = THRESHOLD_CHECK_CNT;
                b->trigger = THRESHOLD_TRIGGER;
                b->degree = THRESHOLD_DEGREE;
                b->step = THRESHOLD_STEP;
                b->ctl_set = builder_dynamic_ctl_set;
                return b;
            default:
                log_err("Unrecognized memory policy %u", policy);
                jemk_free(b);
        }
    }
    return NULL;
}
// clang-format on

MEMKIND_EXPORT void memtier_builder_delete(struct memtier_builder *builder)
{
    print_builder(builder);
    jemk_free(builder->thres);
    jemk_free(builder->cfg);
    jemk_free(builder);
}

MEMKIND_EXPORT int memtier_builder_add_tier(struct memtier_builder *builder,
                                            memkind_t kind, unsigned kind_ratio)
{
    int i;

    if (!kind) {
        log_err("Kind is empty.");
        return -1;
    }

    for (i = 0; i < builder->cfg_size; ++i) {
        if (kind == builder->cfg[i].kind) {
            log_err("Kind is already in builder.");
            return -1;
        }
    }

    struct memtier_tier_cfg *cfg =
        jemk_realloc(builder->cfg, sizeof(*cfg) * (builder->cfg_size + 1));

    if (!cfg) {
        log_err("realloc() failed.");
        return -1;
    }

    builder->cfg = cfg;
    builder->cfg[builder->cfg_size].kind = kind;
    builder->cfg[builder->cfg_size].kind_ratio = kind_ratio;
    builder->cfg_size += 1;
    return 0;
}

static int memtier_policy_dynamic_threshold_construct_memtier_memory(
    struct memtier_builder *builder, struct memtier_memory *memory)
{
    int i;

    if (builder->cfg_size < 2) {
        log_err("There should be at least 2 tiers added to builder "
                "to use POLICY_DYNAMIC_THRESHOLD");
        return -1;
    }

    memory->thres = jemk_calloc(THRESHOLD_NUM(builder),
                                sizeof(struct memtier_threshold_cfg));
    if (!memory->thres) {
        log_err("calloc() failed.");
        return -1;
    }

    memory->thres_init_check_cnt = builder->check_cnt;
    memory->thres_check_cnt = builder->check_cnt;
    memory->thres_trigger = builder->trigger;
    memory->thres_degree = builder->degree;

    for (i = 0; i < builder->thres_size; ++i) {
        memory->thres[i].val = builder->thres[i].val;
        memory->thres[i].min = builder->thres[i].min;
        memory->thres[i].max = builder->thres[i].max;
    }

    // if there are less configured thresholds than tiers, add them now
    if (THRESHOLD_NUM(builder) > builder->thres_size) {
        int old_size = builder->thres_size;
        int last_id = THRESHOLD_NUM(builder) - 1;
        int ret = memtier_builder_create_threshold(builder, last_id);
        if (ret != 0) {
            goto failure;
        }

        for (i = old_size; i < builder->thres_size; ++i) {
            memory->thres[i].val = builder->thres[i].val;
            memory->thres[i].min = builder->thres[i].min;
            memory->thres[i].max = builder->thres[i].max;
        }
    }

    for (i = 0; i < builder->thres_size; ++i) {
        memory->thres[i].norm_ratio =
            builder->cfg[i + 1].kind_ratio / builder->cfg[i].kind_ratio;
    }

    // Validate threshold configuration:
    // * check if values of thresholds are in ascending order - each Nth
    //   threshold value has to be lower than (N+1)th value
    // * each threshold value has to be greater than min and lower than max
    //   value defined for this thresholds
    // * min/max ranges of adjacent threshold should not overlap - max
    //   value of Nth threshold has to be lower than min value of (N+1)th
    //   threshold
    // * threshold trigger and change values has to be positive values
    for (i = 0; i < builder->thres_size; ++i) {
        if (memory->thres[i].min > memory->thres[i].val) {
            log_err("Minimum value of threshold %d "
                    "is too high (min = %zu, val = %zu)",
                    i, memory->thres[i].min, memory->thres[i].val);
            goto failure;
        } else if (memory->thres[i].val > memory->thres[i].max) {
            log_err("Maximum value of threshold %d "
                    "is too low (val = %zu, max = %zu)",
                    i, memory->thres[i].val, memory->thres[i].max);
            goto failure;
        }

        if ((i > 0) && (memory->thres[i - 1].max > memory->thres[i].min)) {
            log_err("Maximum value of threshold %d "
                    "should be less than minimum value of threshold %d",
                    i - 1, i);
            goto failure;
        }
    }

    if (memory->thres_degree < 0) {
        log_err("Threshold change value has to be >= 0");
        goto failure;
    }

    if (memory->thres_trigger < 0) {
        log_err("Threshold trigger value has to be >= 0");
        goto failure;
    }

    return 0;

failure:
    jemk_free(memory->thres);
    return -1;
}

MEMKIND_EXPORT struct memtier_memory *
memtier_builder_construct_memtier_memory(struct memtier_builder *builder)
{
    unsigned i;
    struct memtier_memory *memory;

    if (builder->cfg_size == 0) {
        log_err("No tier in builder.");
        return NULL;
    }

    memory = jemk_malloc(sizeof(struct memtier_memory));
    if (!memory) {
        log_err("malloc() failed.");
        return NULL;
    }

    // perform deep copy but store normalized (to kind[0]) ratio instead of
    // original
    memory->cfg =
        jemk_calloc(builder->cfg_size, sizeof(struct memtier_tier_cfg));
    if (!memory->cfg) {
        log_err("calloc() failed.");
        goto failure_calloc;
    }

    if (builder->policy == MEMTIER_POLICY_DYNAMIC_THRESHOLD) {
        int ret = memtier_policy_dynamic_threshold_construct_memtier_memory(
            builder, memory);
        if (ret != 0) {
            goto failure_cfg;
        }
        memory->get_kind = memtier_policy_dynamic_threshold_get_kind;
        memory->update_cfg = memtier_policy_dynamic_threshold_update_config;
    } else {
        memory->thres = NULL;
        if (builder->cfg_size == 1)
            memory->get_kind = memtier_single_get_kind;
        else {
            memory->get_kind = memtier_policy_static_threshold_get_kind;
        }
        memory->update_cfg = memtier_policy_static_threshold_update_config;
    }

    for (i = 1; i < builder->cfg_size; ++i) {
        memory->cfg[i].kind = builder->cfg[i].kind;
        memory->cfg[i].kind_ratio =
            builder->cfg[0].kind_ratio / builder->cfg[i].kind_ratio;
    }
    memory->cfg[0].kind = builder->cfg[0].kind;
    memory->cfg[0].kind_ratio = 1.0;

    memory->cfg_size = builder->cfg_size;
    return memory;

failure_cfg:
    jemk_free(memory->cfg);

failure_calloc:
    jemk_free(memory);

    return NULL;
}

MEMKIND_EXPORT void memtier_delete_memtier_memory(struct memtier_memory *memory)
{
    print_memtier_memory(memory);
    jemk_free(memory->thres);
    jemk_free(memory->cfg);
    jemk_free(memory);
}

// TODO - create "get" version for builder
// TODO - create "get" version for memtier_memory obj (this will be read-only)
// TODO - how to validate val type? e.g. provide function with explicit size_t
//        type of val for thresholds[ID].val/min/max
MEMKIND_EXPORT int memtier_ctl_set(struct memtier_builder *builder,
                                   const char *name, const void *val)
{
    return builder->ctl_set(builder, name, val);
}

MEMKIND_EXPORT void *memtier_malloc(struct memtier_memory *memory, size_t size)
{
    void *ptr = memtier_kind_malloc(memory->get_kind(memory, size), size);
    memory->update_cfg(memory);

    return ptr;
}

MEMKIND_EXPORT void *memtier_kind_malloc(memkind_t kind, size_t size)
{
    void *ptr = memkind_malloc(kind, size);
    increment_alloc_size(kind->partition, jemk_malloc_usable_size(ptr));
    return ptr;
}

MEMKIND_EXPORT void *memtier_calloc(struct memtier_memory *memory, size_t num,
                                    size_t size)
{
    void *ptr = memtier_kind_calloc(memory->get_kind(memory, size), num, size);
    memory->update_cfg(memory);

    return ptr;
}

MEMKIND_EXPORT void *memtier_kind_calloc(memkind_t kind, size_t num,
                                         size_t size)
{
    void *ptr = memkind_calloc(kind, num, size);
    increment_alloc_size(kind->partition, jemk_malloc_usable_size(ptr));
    return ptr;
}

MEMKIND_EXPORT void *memtier_realloc(struct memtier_memory *memory, void *ptr,
                                     size_t size)
{
    // reallocate inside same kind
    if (ptr) {
        struct memkind *kind = memkind_detect_kind(ptr);
        ptr = memtier_kind_realloc(kind, ptr, size);
        memory->update_cfg(memory);

        return ptr;
    }

    ptr = memtier_kind_malloc(memory->get_kind(memory, size), size);
    memory->update_cfg(memory);

    return ptr;
}

MEMKIND_EXPORT void *memtier_kind_realloc(memkind_t kind, void *ptr,
                                          size_t size)
{
    if (size == 0 && ptr != NULL) {
        decrement_alloc_size(kind->partition, jemk_malloc_usable_size(ptr));
        memkind_free(kind, ptr);
        return NULL;
    } else if (ptr == NULL) {
        void *n_ptr = memkind_malloc(kind, size);
        increment_alloc_size(kind->partition, jemk_malloc_usable_size(n_ptr));
        return n_ptr;
    } else {
        decrement_alloc_size(kind->partition, jemk_malloc_usable_size(ptr));

        void *n_ptr = memkind_realloc(kind, ptr, size);
        increment_alloc_size(kind->partition, jemk_malloc_usable_size(n_ptr));
        return n_ptr;
    }
}

MEMKIND_EXPORT int memtier_posix_memalign(struct memtier_memory *memory,
                                          void **memptr, size_t alignment,
                                          size_t size)
{
    int ret = memtier_kind_posix_memalign(memory->get_kind(memory, size),
                                          memptr, alignment, size);
    memory->update_cfg(memory);

    return ret;
}

MEMKIND_EXPORT int memtier_kind_posix_memalign(memkind_t kind, void **memptr,
                                               size_t alignment, size_t size)
{
    int res = memkind_posix_memalign(kind, memptr, alignment, size);
    increment_alloc_size(kind->partition, jemk_malloc_usable_size(*memptr));
    return res;
}

MEMKIND_EXPORT size_t memtier_usable_size(void *ptr)
{
    return jemk_malloc_usable_size(ptr);
}

MEMKIND_EXPORT void memtier_kind_free(memkind_t kind, void *ptr)
{
    if (!kind) {
        kind = memkind_detect_kind(ptr);
        if (!kind)
            return;
    }
    decrement_alloc_size(kind->partition, jemk_malloc_usable_size(ptr));
    memkind_free(kind, ptr);
}

MEMKIND_EXPORT size_t memtier_kind_allocated_size(memkind_t kind)
{
    size_t size_ret;
    long long size_all = 0;
    long long size;
    unsigned bucket_id;

    for (bucket_id = 0; bucket_id < THREAD_BUCKETS; ++bucket_id) {
        size = memkind_atomic_get_and_zeroing(
            t_alloc_size[kind->partition][bucket_id]);
        size_all += size;
    }
    size_ret =
        memkind_atomic_increment(g_alloc_size[kind->partition], size_all);
    return (size_ret + size_all);
}
