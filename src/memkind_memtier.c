// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind_memtier.h>

#include <memkind/internal/memkind_arena.h>

MEMKIND_EXPORT struct memtier_tier *memtier_tier_new(void)
{
    return jemk_malloc(sizeof(struct memtier_tier));
}

MEMKIND_EXPORT void memtier_tier_delete(struct memtier_tier *tier)
{
    jemk_free(tier);
}

MEMKIND_EXPORT int memtier_tier_set_ratio(struct memtier_tier *tier,
                                          size_t ratio)
{
    tier->ratio = ratio;
    return 0;
}

MEMKIND_EXPORT int memtier_tier_set_memory_kind(struct memtier_tier *tier,
                                                memkind_t kind)
{
    tier->kind = kind;
    return 0;
}

MEMKIND_EXPORT struct memtier_builder *memtier_builder(void)
{
    return jemk_malloc(sizeof(struct memtier_builder));
}

MEMKIND_EXPORT int memtier_builder_add_tier(struct memtier_builder *cfg,
                                            struct memtier_tier *tier)
{
    // TODO provide adding tiering logic
    return 0;
}

MEMKIND_EXPORT int memtier_builder_set_policy(struct memtier_builder *cfg,
                                              memtier_policy_t policy)
{
    // TODO provide setting policy logic
    return 0;
}

MEMKIND_EXPORT int
memtier_builder_construct_kind(struct memtier_builder *builder,
                               struct memtier_kind **kind)
{
    *kind = (struct memtier_kind *)jemk_malloc(sizeof(struct memtier_kind));
    jemk_free(builder);
    return 0;
}

MEMKIND_EXPORT void memtier_delete_kind(struct memtier_kind *kind)
{
    jemk_free(kind);
}

MEMKIND_EXPORT void *memtier_kind_malloc(struct memtier_kind *kind, size_t size)
{
    // TODO provide tiering_kind logic
    return NULL;
}

MEMKIND_EXPORT void *memtier_tier_malloc(struct memtier_tier *tier, size_t size)
{
    // TODO provide increment counter logic
    return memkind_malloc(tier->kind, size);
}

MEMKIND_EXPORT void *memtier_kind_calloc(struct memtier_kind *kind, size_t num,
                                         size_t size)
{
    // TODO provide tiering_kind logic
    return NULL;
}

MEMKIND_EXPORT void *memtier_tier_calloc(struct memtier_tier *tier, size_t num,
                                         size_t size)
{
    // TODO provide increment counter logic
    return memkind_calloc(tier->kind, num, size);
}

MEMKIND_EXPORT void *memtier_kind_realloc(struct memtier_kind *kind, void *ptr,
                                          size_t size)
{
    // TODO provide tiering_kind logic
    return NULL;
}

MEMKIND_EXPORT void *memtier_tier_realloc(struct memtier_tier *tier, void *ptr,
                                          size_t size)
{
    if (size == 0 && ptr != NULL) {
        memkind_free(tier->kind, ptr);
        // TODO provide decrement counter logic
        return NULL;
    } else if (ptr == NULL) {
        // TODO provide increment counter logic
        return memkind_malloc(tier->kind, size);
    } else {
        size_t prev_size = memkind_malloc_usable_size(tier->kind, ptr);
        (void)(prev_size);
        // TODO provide increment/decrement counter logic
        return memkind_realloc(tier->kind, ptr, size);
    }
}

MEMKIND_EXPORT int memtier_kind_posix_memalign(struct memtier_kind *kind,
                                               void **memptr, size_t alignment,
                                               size_t size)
{
    // TODO provide tiering_kind logic
    return 0;
}

MEMKIND_EXPORT int memtier_tier_posix_memalign(struct memtier_tier *tier,
                                               void **memptr, size_t alignment,
                                               size_t size)
{
    // TODO provide increment counter logic
    return memkind_posix_memalign(tier->kind, memptr, alignment, size);
}

MEMKIND_EXPORT size_t memtier_usable_size(void *ptr)
{
    return memkind_malloc_usable_size(NULL, ptr);
}

MEMKIND_EXPORT void memtier_free(void *ptr)
{
    struct memkind *kind = memkind_detect_kind(ptr);
    // TODO detect tier from which kind belongs
    // TODO provide increment/decrement counter logic
    memkind_free(kind, ptr);
}
