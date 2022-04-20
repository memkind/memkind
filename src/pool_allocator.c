// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/pool_allocator.h"
#include "memkind/internal/hasher.h"
#include "memkind/internal/memkind_private.h"
#include "memkind/internal/pool_allocator_internal_utils.h"

#include "stdbool.h"
#include "string.h"

// -------- typedefs ----------------------------------------------------------

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#else
#define atomic_load(object) __atomic_load_n(object, __ATOMIC_SEQ_CST)
#define atomic_compare_exchange_strong(object, expected, desired)              \
    __atomic_compare_exchange((object), (expected), &(desired), false,         \
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#endif

// -------- public functions --------------------------------------------------

MEMKIND_EXPORT void *pool_allocator_malloc(PoolAllocator *pool, size_t size)
{
    if (size == 0)
        return NULL;
    size_t size_rank = size_to_rank_size(size);
    uint16_t hash = hasher_calculate_hash(size_rank);
    SlabAllocator *slab = pool->pool[hash];
    if (!slab) {
        SlabAllocator *null_slab = slab;
        // TODO initialize the slab in a lockless way
        slab = slab_allocator_malloc(&pool->slabSlabAllocator);
        size_t slab_size = rank_size_to_size(size_rank);
        int ret = slab_allocator_init((SlabAllocator *)slab, slab_size, 0);
        if (ret != 0)
            return NULL;
        bool exchanged =
            atomic_compare_exchange_strong(&pool->pool[hash], &null_slab, slab);
        if (!exchanged) {
            slab_allocator_destroy(slab);
            slab = atomic_load(&pool->pool[hash]);
        }
    }

    return slab_allocator_malloc(slab);
}

MEMKIND_EXPORT void *pool_allocator_realloc(PoolAllocator *pool, void *ptr,
                                            size_t size)
{
    if (size == 0) {
        pool_allocator_free(ptr);
        return NULL;
    }
    if (ptr == NULL) {
        return pool_allocator_malloc(pool, size);
    }

    size_t current_size = slab_allocator_usable_size(ptr);
    if (size_to_rank_size(size) == current_size) {
        return ptr;
    }

    void *ret = pool_allocator_malloc(pool, size);
    if (ret) {
        size_t to_copy = size < current_size ? size : current_size;
        memcpy(ret, ptr, to_copy);
        pool_allocator_free(ptr);
    }

    return ret;
}

MEMKIND_EXPORT void pool_allocator_free(void *ptr)
{
    slab_allocator_free(ptr);
}

MEMKIND_EXPORT size_t pool_allocator_usable_size(void *ptr)
{
    return slab_allocator_usable_size(ptr);
}

MEMKIND_EXPORT int pool_allocator_create(PoolAllocator *pool)
{
    int ret = slab_allocator_init(&pool->slabSlabAllocator,
                                  sizeof(SlabAllocator), UINT16_MAX);
    if (ret == 0)
        (void)memset(pool->pool, 0, sizeof(pool->pool));

    return ret;
}

MEMKIND_EXPORT void pool_allocator_destroy(PoolAllocator *pool)
{
    for (size_t i = 0; i < UINT16_MAX; ++i)
        slab_allocator_free(pool->pool[i]);
    slab_allocator_destroy(&pool->slabSlabAllocator);
}
