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

struct memtier_tier_cfg {
    memkind_t kind;   // Memory kind
    float kind_ratio; // Memory tier ratio
};

struct memtier_builder {
    unsigned size;                // Number of memory kinds
    memtier_policy_t policy;      // Tiering policy
    struct memtier_tier_cfg *cfg; // Memory Tier configuration
};

struct memtier_kind {
    unsigned size;                // Number of memory kinds
    memtier_policy_t policy;      // Tiering policy
    struct memtier_tier_cfg *cfg; // Memory Tier configuration
};

MEMKIND_ATOMIC size_t kind_alloc_size[MEMKIND_MAX_KIND];

void memtier_reset_size(unsigned id)
{
    kind_alloc_size[id] = 0;
}

static memkind_t
memtier_policy_static_threshold_get_kind(struct memtier_kind *tier_kind)
{
    struct memtier_tier_cfg *cfg = tier_kind->cfg;

    int i;
    int dest_kind = 0;
    for (i = 1; i < tier_kind->size; ++i) {
        if ((kind_alloc_size[cfg[i].kind->partition] * cfg[i].kind_ratio) <
            kind_alloc_size[cfg[0].kind->partition]) {
            dest_kind = i;
        }
    }

    return cfg[dest_kind].kind;
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
                                            memkind_t kind, unsigned kind_ratio)
{
    if (!kind) {
        log_err("Kind is empty.");
        return -1;
    }

    struct memtier_tier_cfg *cfg =
        jemk_realloc(builder->cfg, sizeof(*cfg) * (builder->size + 1));

    if (!cfg) {
        log_err("realloc() failed.");
        return -1;
    }

    builder->cfg = cfg;
    builder->cfg[builder->size].kind = kind;
    builder->cfg[builder->size].kind_ratio = kind_ratio;
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

static inline memkind_t get_tier(struct memtier_kind *tier_kind)
{

    if (tier_kind->policy == MEMTIER_POLICY_STATIC_THRESHOLD) {
        return memtier_policy_static_threshold_get_kind(tier_kind);
    }

    log_err("Unrecognized memory policy %u", tier_kind->policy);
    return NULL;
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

    *kind = jemk_malloc(sizeof(struct memtier_kind));
    if (!*kind) {
        log_err("malloc() failed.");
        return -1;
    }

    // perform deep copy but store normalized (to kind[0]) ratio instead of
    // original
    (*kind)->cfg = jemk_calloc(builder->size, sizeof(struct memtier_tier_cfg));
    if (!(*kind)->cfg) {
        log_err("calloc() failed.");
        goto free_kind;
    }

    for (i = 1; i < builder->size; ++i) {
        (*kind)->cfg[i].kind = builder->cfg[i].kind;
        (*kind)->cfg[i].kind_ratio =
            builder->cfg[0].kind_ratio / builder->cfg[i].kind_ratio;
    }
    (*kind)->cfg[0].kind = builder->cfg[0].kind;
    (*kind)->cfg[0].kind_ratio = 1.0;

    (*kind)->size = builder->size;
    (*kind)->policy = builder->policy;
    return 0;

free_kind:
    jemk_free(*kind);

    return -1;
}

MEMKIND_EXPORT void memtier_delete_kind(struct memtier_kind *kind)
{
    jemk_free(kind->cfg);
    jemk_free(kind);
}

MEMKIND_EXPORT void *memtier_kind_malloc(struct memtier_kind *kind, size_t size)
{
    return memtier_tier_malloc(get_tier(kind), size);
}

MEMKIND_EXPORT void *memtier_tier_malloc(memkind_t kind, size_t size)
{
    void *ptr = memkind_malloc(kind, size);
    memkind_atomic_increment(kind_alloc_size[kind->partition],
                             jemk_malloc_usable_size(ptr));
    return ptr;
}

MEMKIND_EXPORT void *memtier_kind_calloc(struct memtier_kind *kind, size_t num,
                                         size_t size)
{
    return memtier_tier_calloc(get_tier(kind), num, size);
}

MEMKIND_EXPORT void *memtier_tier_calloc(memkind_t kind, size_t num,
                                         size_t size)
{
    void *ptr = memkind_calloc(kind, num, size);
    memkind_atomic_increment(kind_alloc_size[kind->partition],
                             jemk_malloc_usable_size(ptr));
    return ptr;
}

MEMKIND_EXPORT void *memtier_kind_realloc(struct memtier_kind *kind, void *ptr,
                                          size_t size)
{
    // reallocate inside same kind
    if (ptr) {
        struct memkind *kind = memkind_detect_kind(ptr);
        return memtier_tier_realloc(kind, ptr, size);
    }
    return memtier_tier_malloc(get_tier(kind), size);
}

MEMKIND_EXPORT void *memtier_tier_realloc(memkind_t kind, void *ptr,
                                          size_t size)
{
    if (size == 0 && ptr != NULL) {
        memkind_atomic_decrement(kind_alloc_size[kind->partition],
                                 jemk_malloc_usable_size(ptr));
        memkind_free(kind, ptr);
        return NULL;
    } else if (ptr == NULL) {
        void *n_ptr = memkind_malloc(kind, size);
        memkind_atomic_increment(kind_alloc_size[kind->partition],
                                 jemk_malloc_usable_size(n_ptr));
        return n_ptr;
    } else {
        memkind_atomic_decrement(kind_alloc_size[kind->partition],
                                 jemk_malloc_usable_size(ptr));
        void *n_ptr = memkind_realloc(kind, ptr, size);
        memkind_atomic_increment(kind_alloc_size[kind->partition],
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

MEMKIND_EXPORT int memtier_tier_posix_memalign(memkind_t kind, void **memptr,
                                               size_t alignment, size_t size)
{
    int res = memkind_posix_memalign(kind, memptr, alignment, size);
    memkind_atomic_increment(kind_alloc_size[kind->partition],
                             jemk_malloc_usable_size(*memptr));
    return res;
}

MEMKIND_EXPORT size_t memtier_usable_size(void *ptr)
{
    return jemk_malloc_usable_size(ptr);
}

MEMKIND_EXPORT void memtier_free(void *ptr)
{
    memkind_t kind = memkind_detect_kind(ptr);
    if (!kind)
        return;
    memkind_atomic_decrement(kind_alloc_size[kind->partition],
                             jemk_malloc_usable_size(ptr));
    memkind_free(kind, ptr);
}

MEMKIND_EXPORT size_t memtier_tier_allocated_size(memkind_t kind)
{
    return kind_alloc_size[kind->partition];
}
