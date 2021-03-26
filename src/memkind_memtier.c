// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind_memtier.h>

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_log.h>

#include "config.h"
#include <assert.h>

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#define MEMKIND_ATOMIC _Atomic
#else
#define MEMKIND_ATOMIC
#endif

#if defined(MEMKIND_ATOMIC_C11_SUPPORT)
#define memkind_atomic_increment(counter, val)                                 \
    atomic_fetch_add_explicit(&counter, val, memory_order_relaxed)
#define memkind_atomic_decrement(counter, val)                                 \
    atomic_fetch_sub_explicit(&counter, val, memory_order_relaxed)
#elif defined(MEMKIND_ATOMIC_BUILTINS_SUPPORT)
#define memkind_atomic_increment(counter, val)                                 \
    __atomic_add_fetch(&counter, val, __ATOMIC_RELAXED)
#define memkind_atomic_decrement(counter, val)                                 \
    __atomic_sub_fetch(&counter, val, __ATOMIC_RELAXED)
#elif defined(MEMKIND_ATOMIC_SYNC_SUPPORT)
#define memkind_atomic_increment(counter, val)                                 \
    __sync_add_and_fetch(&counter, val)
#define memkind_atomic_decrement(counter, val)                                 \
    __sync_sub_and_fetch(&counter, val)
#else
#error "Missing atomic implementation."
#endif

struct memtier_tier {
    memkind_t kind;                   // Memory kind
    MEMKIND_ATOMIC size_t alloc_size; // Allocated size
};

struct memtier_tier_cfg {
    struct memtier_tier *tier; // Memory tier
    unsigned tier_ratio;       // Memory tier ratio
};

struct memtier_builder {
    unsigned size;                // Number of memory tiers
    memtier_policy_t policy;      // Tiering policy
    struct memtier_tier_cfg *cfg; // Memory Tier configuration
};

struct memtier_policy {
    int (*create_priv)(struct memtier_kind *tier_kind);
    void (*free_priv)(struct memtier_kind *tier_kind);
    struct memtier_tier *(*get_tier)(struct memtier_kind *tier_kind);
};

struct memtier_kind {
    unsigned size;                // Number of memory tiers
    struct memtier_tier_cfg *cfg; // Memory Tier configuration
    struct memtier_policy policy; // Tiering policy
    void *policy_priv;            // Policy private data
};

struct static_threshold_tier_data {
    float normalized_ratio;
    float actual_ratio;
};

int memtier_policy_static_threshold_create(struct memtier_kind *tier_kind)
{
    unsigned int i;

    tier_kind->policy_priv =
        calloc(tier_kind->size, sizeof(struct static_threshold_tier_data));
    if (tier_kind->policy_priv == NULL) {
        return -1;
    }

    struct static_threshold_tier_data *tiers =
        (struct static_threshold_tier_data *)tier_kind->policy_priv;

    // calculate and store normalized (vs tier[0]) tier ratios
    for (i = 0; i < tier_kind->size; ++i) {
        tiers[i].normalized_ratio =
            tier_kind->cfg[i].tier_ratio / tier_kind->cfg[0].tier_ratio;
    }
    tiers[0].normalized_ratio = 1;
    tiers[0].actual_ratio = 1;

    return 0;
}

void memtier_policy_static_threshold_free(struct memtier_kind *kind)
{
    jemk_free(kind->policy_priv);
}

struct memtier_tier *
memtier_policy_static_threshold_get_tier(struct memtier_kind *tier_kind)
{
    struct static_threshold_tier_data *tiers = tier_kind->policy_priv;
    struct memtier_tier_cfg *cfg = tier_kind->cfg;

    // algorithm:
    // * get allocation sizes from all tiers
    // * calculate actual ratio between tiers
    // * choose tier with largest difference between actual and desired ratio

    int i;
    int worse_tier = -1;
    float worse_ratio;
    for (i = 0; i < tier_kind->size; ++i) {
        tiers[i].actual_ratio =
            (float)cfg[i].tier->alloc_size / cfg[0].tier->alloc_size;
        float ratio_distance = (float)tiers[i].actual_ratio / cfg[i].tier_ratio;
        if ((worse_tier == -1) || (cfg[i].tier->alloc_size == 0) ||
            (ratio_distance < worse_ratio)) {
            worse_tier = i;
            worse_ratio = ratio_distance;
        }
    }

    return cfg[worse_tier].tier;
}

struct memtier_policy MEMTIER_POLICY_STATIC_THRESHOLD_OBJ = {
    .create_priv = memtier_policy_static_threshold_create,
    .free_priv = memtier_policy_static_threshold_free,
    .get_tier = memtier_policy_static_threshold_get_tier,
};

// Provide translation from memkind_t to memtier_t
// memkind_t partition id -> memtier tier
struct memtier_registry {
    struct memtier_tier *kind_map[MEMKIND_MAX_KIND];
    pthread_mutex_t lock;
};

static struct memtier_registry memtier_registry_g = {
    {NULL},
    PTHREAD_MUTEX_INITIALIZER,
};

MEMKIND_EXPORT struct memtier_tier *memtier_tier_new(memkind_t kind)
{
    if (pthread_mutex_lock(&memtier_registry_g.lock) != 0)
        assert(0 && "failed to acquire mutex");
    if (!kind || memtier_registry_g.kind_map[kind->partition]) {
        log_err("Kind is empty or tier with kind already exists.");
        if (pthread_mutex_unlock(&memtier_registry_g.lock) != 0)
            assert(0 && "failed to release mutex");
        return NULL;
    }

    struct memtier_tier *tier = jemk_malloc(sizeof(*tier));
    if (tier) {
        tier->kind = kind;
        tier->alloc_size = 0;
        memtier_registry_g.kind_map[kind->partition] = tier;
    }
    if (pthread_mutex_unlock(&memtier_registry_g.lock) != 0)
        assert(0 && "failed to release mutex");
    return tier;
}

MEMKIND_EXPORT void memtier_tier_delete(struct memtier_tier *tier)
{
    if (tier) {
        if (pthread_mutex_lock(&memtier_registry_g.lock) != 0)
            assert(0 && "failed to acquire mutex");
        memtier_registry_g.kind_map[tier->kind->partition] = NULL;
        if (pthread_mutex_unlock(&memtier_registry_g.lock) != 0)
            assert(0 && "failed to release mutex");
    }
    jemk_free(tier);
}

MEMKIND_EXPORT struct memtier_builder *memtier_builder_new(void)
{
    return jemk_calloc(1, sizeof(struct memtier_builder));
}

MEMKIND_EXPORT void memtier_builder_delete(struct memtier_builder *builder)
{
    jemk_free(builder->cfg);
    jemk_free(builder);
}

MEMKIND_EXPORT int memtier_builder_add_tier(struct memtier_builder *builder,
                                            struct memtier_tier *tier,
                                            unsigned tier_ratio)
{
    if (!tier) {
        log_err("Tier is empty.");
        return -1;
    }

    struct memtier_tier_cfg *cfg =
        jemk_realloc(builder->cfg, sizeof(*cfg) * (builder->size + 1));

    if (!cfg) {
        log_err("realloc() failed.");
        return -1;
    }

    builder->cfg = cfg;
    builder->cfg[builder->size].tier = tier;
    builder->cfg[builder->size].tier_ratio = tier_ratio;
    builder->size += 1;
    return 0;
}

MEMKIND_EXPORT int memtier_builder_set_policy(struct memtier_builder *builder,
                                              memtier_policy_t policy)
{
    if (policy == MEMTIER_POLICY_STATIC_THRESHOLD) {
        builder->policy = policy;
    } else {
        log_err("Unrecognized memory policy %u", policy);
        return -1;
    }

    return 0;
}

static inline struct memtier_tier *get_tier(struct memtier_kind *tier_kind)
{
    return tier_kind->policy.get_tier(tier_kind);
}

MEMKIND_EXPORT int
memtier_builder_construct_kind(struct memtier_builder *builder,
                               struct memtier_kind **kind)
{
    unsigned i;
    if (builder->size == 0) {
        log_err("No tier in builder.");
        return -1;
    }

    *kind = jemk_malloc(sizeof(sizeof(struct memtier_kind)));
    if (!*kind) {
        log_err("malloc() failed.");
        goto free_kind;
    }

    // perform deep copy
    (*kind)->cfg = jemk_calloc(builder->size, sizeof(struct memtier_tier_cfg));
    if (!(*kind)->cfg) {
        log_err("calloc() failed.");
        goto free_cfg;
    }

    for (i = 0; i < builder->size; ++i) {
        (*kind)->cfg[i].tier = builder->cfg[i].tier;
        (*kind)->cfg[i].tier_ratio = builder->cfg[i].tier_ratio;
    }

    (*kind)->size = builder->size;

    if (builder->policy == MEMTIER_POLICY_STATIC_THRESHOLD) {
        (*kind)->policy = MEMTIER_POLICY_STATIC_THRESHOLD_OBJ;
    }
    int ret = (*kind)->policy.create_priv(*kind);
    if (ret != 0) {
        log_err("Creation of policy failed.");
        goto free_cfg;
    }
    return 0;

free_cfg:
    jemk_free((*kind)->cfg);

free_kind:
    jemk_free(*kind);

    return -1;
}

MEMKIND_EXPORT void memtier_delete_kind(struct memtier_kind *kind)
{
    kind->policy.free_priv(kind);
    jemk_free(kind->cfg);
    jemk_free(kind);
}

MEMKIND_EXPORT void *memtier_kind_malloc(struct memtier_kind *kind, size_t size)
{
    return memtier_tier_malloc(get_tier(kind), size);
}

MEMKIND_EXPORT void *memtier_tier_malloc(struct memtier_tier *tier, size_t size)
{
    void *ptr = memkind_malloc(tier->kind, size);
    memkind_atomic_increment(tier->alloc_size, jemk_malloc_usable_size(ptr));
    return ptr;
}

MEMKIND_EXPORT void *memtier_kind_calloc(struct memtier_kind *kind, size_t num,
                                         size_t size)
{
    return memtier_tier_calloc(get_tier(kind), num, size);
}

MEMKIND_EXPORT void *memtier_tier_calloc(struct memtier_tier *tier, size_t num,
                                         size_t size)
{
    void *ptr = memkind_calloc(tier->kind, num, size);
    memkind_atomic_increment(tier->alloc_size, jemk_malloc_usable_size(ptr));
    return ptr;
}

MEMKIND_EXPORT void *memtier_kind_realloc(struct memtier_kind *kind, void *ptr,
                                          size_t size)
{
    // reallocate inside same memtier tier
    if (ptr) {
        struct memkind *kind = memkind_detect_kind(ptr);
        struct memtier_tier *tier =
            memtier_registry_g.kind_map[kind->partition];
        return memtier_tier_realloc(tier, ptr, size);
    }
    return memtier_tier_malloc(get_tier(kind), size);
}

MEMKIND_EXPORT void *memtier_tier_realloc(struct memtier_tier *tier, void *ptr,
                                          size_t size)
{
    if (size == 0 && ptr != NULL) {
        memkind_atomic_decrement(tier->alloc_size,
                                 jemk_malloc_usable_size(ptr));
        memkind_free(tier->kind, ptr);
        return NULL;
    } else if (ptr == NULL) {
        void *n_ptr = memkind_malloc(tier->kind, size);
        memkind_atomic_increment(tier->alloc_size,
                                 jemk_malloc_usable_size(n_ptr));
        return n_ptr;
    } else {
        memkind_atomic_decrement(tier->alloc_size,
                                 jemk_malloc_usable_size(ptr));
        void *n_ptr = memkind_realloc(tier->kind, ptr, size);
        memkind_atomic_increment(tier->alloc_size,
                                 jemk_malloc_usable_size(n_ptr));
        return n_ptr;
    }
}

MEMKIND_EXPORT int memtier_kind_posix_memalign(struct memtier_kind *kind,
                                               void **memptr, size_t alignment,
                                               size_t size)
{
    return memtier_tier_posix_memalign(get_tier(kind), memptr, alignment, size);
}

MEMKIND_EXPORT int memtier_tier_posix_memalign(struct memtier_tier *tier,
                                               void **memptr, size_t alignment,
                                               size_t size)
{
    int res = memkind_posix_memalign(tier->kind, memptr, alignment, size);
    memkind_atomic_increment(tier->alloc_size,
                             jemk_malloc_usable_size(*memptr));
    return res;
}

MEMKIND_EXPORT size_t memtier_usable_size(void *ptr)
{
    return jemk_malloc_usable_size(ptr);
}

MEMKIND_EXPORT void memtier_free(void *ptr)
{
    struct memkind *kind = memkind_detect_kind(ptr);
    if (!kind)
        return;
    struct memtier_tier *tier = memtier_registry_g.kind_map[kind->partition];
    memkind_atomic_decrement(tier->alloc_size, jemk_malloc_usable_size(ptr));
    memkind_free(kind, ptr);
}

MEMKIND_EXPORT size_t memtier_tier_allocated_size(struct memtier_tier *tier)
{
    return tier->alloc_size;
}
