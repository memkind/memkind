// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind_memtier.h>

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_log.h>

#include <assert.h>

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
    struct memtier_tier *tier;
    if (pthread_mutex_lock(&memtier_registry_g.lock) != 0)
        assert(0 && "failed to acquire mutex");
    if (!kind || memtier_registry_g.kind_map[kind->partition]) {
        log_err("Kind is empty or tier with kind already exists.");
        if (pthread_mutex_unlock(&memtier_registry_g.lock) != 0)
            assert(0 && "failed to release mutex");
        return NULL;
    }

    tier = jemk_malloc(sizeof(struct memtier_tier));
    if (tier) {
        tier->kind = kind;
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

MEMKIND_EXPORT struct memtier_builder *memtier_builder(void)
{
    return jemk_malloc(sizeof(struct memtier_builder));
}

MEMKIND_EXPORT int memtier_builder_add_tier(struct memtier_builder *builder,
                                            struct memtier_tier *tier,
                                            unsigned tier_ratio)
{
    // TODO provide adding tiering logic
    if (!tier) {
        log_err("Tier is empty.");
        return -1;
    }
    builder->todo_kind = tier->kind;
    return 0;
}

MEMKIND_EXPORT int memtier_builder_set_policy(struct memtier_builder *builder,
                                              memtier_policy_t policy)
{
    // TODO provide setting policy logic
    if (policy == MEMTIER_DUMMY_VALUE)
        return 0;
    log_err("Unrecognized memory policy %u", policy);
    return -1;
}

MEMKIND_EXPORT int
memtier_builder_construct_kind(struct memtier_builder *builder,
                               struct memtier_kind **kind)
{
    *kind = (struct memtier_kind *)jemk_malloc(sizeof(struct memtier_kind));
    if (*kind) {
        (*kind)->todo_kind = builder->todo_kind;
        jemk_free(builder);
        return 0;
    }
    log_err("malloc() failed.");
    return -1;
}

MEMKIND_EXPORT void memtier_delete_kind(struct memtier_kind *kind)
{
    jemk_free(kind);
}

MEMKIND_EXPORT void *memtier_kind_malloc(struct memtier_kind *kind, size_t size)
{
    // TODO provide tiering_kind logic
    void *ptr = memkind_malloc(kind->todo_kind, size);
    log_info("malloc(%zu) = %p from kind: %s", size, ptr,
             kind->todo_kind->name);
    return ptr;
}

MEMKIND_EXPORT void *memtier_tier_malloc(struct memtier_tier *tier, size_t size)
{
    // TODO provide increment counter logic
    void *ptr = memkind_malloc(tier->kind, size);
    log_info("malloc(%zu) = %p from tier: %s", size, ptr, tier->kind->name);
    return ptr;
}

MEMKIND_EXPORT void *memtier_kind_calloc(struct memtier_kind *kind, size_t num,
                                         size_t size)
{
    // TODO provide tiering_kind logic
    void *ptr = memkind_calloc(kind->todo_kind, num, size);
    log_info("calloc(%zu, %zu) = %p from kind: %s", num, size, ptr,
             kind->todo_kind->name);
    return ptr;
}

MEMKIND_EXPORT void *memtier_tier_calloc(struct memtier_tier *tier, size_t num,
                                         size_t size)
{
    // TODO provide increment counter logic
    void *ptr = memkind_calloc(tier->kind, num, size);
    log_info("calloc(%zu, %zu) = %p from kind: %s", num, size, ptr,
             tier->kind->name);
    return ptr;
}

MEMKIND_EXPORT void *memtier_kind_realloc(struct memtier_kind *kind, void *ptr,
                                          size_t size)
{
    // TODO provide tiering_kind logic
    void *ret = memkind_realloc(kind->todo_kind, ptr, size);
    log_info("realloc(%p, %zu) = %p from kind: %s", ptr, size, ret,
             kind->todo_kind->name);
    return ret;
}

MEMKIND_EXPORT void *memtier_tier_realloc(struct memtier_tier *tier, void *ptr,
                                          size_t size)
{
    void *ret = NULL;
    if (size == 0 && ptr != NULL) {
        log_info("free(%p) from kind: %s", ptr, tier->kind->name);
        memkind_free(tier->kind, ptr);
        // TODO provide decrement counter logic
        return ret;
    } else if (ptr == NULL) {
        // TODO provide increment counter logic
        log_info("malloc(%zu) = %p from tier: %s", size, ptr, tier->kind->name);
        ret = memkind_malloc(tier->kind, size);
    } else {
        size_t prev_size = memkind_malloc_usable_size(tier->kind, ptr);
        (void)(prev_size);
        // TODO provide increment/decrement counter logic
        log_info("realloc(%p, %zu) = %p from kind: %s", ptr, size, ret,
                 tier->kind->name);
        ret = memkind_realloc(tier->kind, ptr, size);
    }

    log_info("realloc(%p, %zu) = %p from kind: %s", ptr, size, ret,
             tier->kind->name);
    return ret;
}

MEMKIND_EXPORT int memtier_kind_posix_memalign(struct memtier_kind *kind,
                                               void **memptr, size_t alignment,
                                               size_t size)
{
    // TODO provide tiering_kind logic
    int ret = memkind_posix_memalign(kind->todo_kind, memptr, alignment, size);
    log_info("posix_memalign(%p, %zu, %zu) = %d from kind: %s", memptr,
             alignment, size, ret, kind->todo_kind->name);
    return ret;
}

MEMKIND_EXPORT int memtier_tier_posix_memalign(struct memtier_tier *tier,
                                               void **memptr, size_t alignment,
                                               size_t size)
{
    // TODO provide increment counter logic
    int ret = memkind_posix_memalign(tier->kind, memptr, alignment, size);
    log_info("posix_memalign(%p, %zu, %zu) = %d from kind: %s", memptr,
             alignment, size, ret, tier->kind->name);
    return ret;
}

MEMKIND_EXPORT size_t memtier_usable_size(void *ptr)
{
    return memkind_malloc_usable_size(NULL, ptr);
}

MEMKIND_EXPORT void memtier_free(void *ptr)
{
    struct memkind *kind = memkind_detect_kind(ptr);
    if (!kind)
        return;
    struct memtier_tier *tier = memtier_registry_g.kind_map[kind->partition];
    (void)(tier);
    // TODO provide increment/decrement counter logic
    log_info("free(%p) from kind: %s", ptr, kind->name);
    memkind_free(kind, ptr);
}
