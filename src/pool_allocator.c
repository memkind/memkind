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
    size_t size_rank = size_to_size_rank(size);
    uint16_t hash = hasher_calculate_hash(size_rank);
    SlabAllocator *slab = pool->pool[hash];
    if (!slab) {
        SlabAllocator *null_slab = slab;
        // TODO initialize the slab in a lockless way
        slab = slab_allocator_malloc(&pool->slabSlabAllocator);
        size_t slab_size = size_rank_to_size(size_rank);
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

MEMKIND_EXPORT void *pool_allocator_malloc_pages(PoolAllocator *pool,
                                                 size_t size,
                                                 uintptr_t *address,
                                                 size_t *nof_pages)
{
    if (size == 0)
        return NULL;
    size_t size_rank = size_to_size_rank(size);
    uint16_t hash = hasher_calculate_hash(size_rank);
    SlabAllocator *slab = pool->pool[hash];
    if (!slab) {
        SlabAllocator *null_slab = slab;
        // TODO initialize the slab in a lockless way
        slab = slab_allocator_malloc(&pool->slabSlabAllocator);
        size_t slab_size = size_rank_to_size(size_rank);
        int ret = slab_allocator_init(slab, slab_size, 0);
        if (ret != 0)
            return NULL;
        // TODO atomic compare exchange WARNING HACK NOT THREAD SAFE NOW !!!!
        bool exchanged =
            atomic_compare_exchange_strong(&pool->pool[hash], &null_slab, slab);
        if (!exchanged) {
            slab_allocator_destroy(slab);
            slab = atomic_load(&pool->pool[hash]);
        }
    }

    return slab_allocator_malloc_pages(slab, address, nof_pages);
}

MEMKIND_EXPORT void *pool_allocator_realloc(PoolAllocator *pool, void *ptr,
                                            size_t size)
{
    pool_allocator_free(ptr);
    return pool_allocator_malloc(pool, size);
}

MEMKIND_EXPORT void *pool_allocator_realloc_pages(PoolAllocator *pool,
                                                  void *ptr, size_t size,
                                                  uintptr_t *addr,
                                                  size_t *nof_pages)
{
    pool_allocator_free(ptr);
    return pool_allocator_malloc_pages(pool, size, addr, nof_pages);
}

MEMKIND_EXPORT void pool_allocator_free(void *ptr)
{
    slab_allocator_free(ptr);
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
