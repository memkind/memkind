// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/fast_pool_allocator.h"
#include "memkind/internal/hasher.h"
#include "memkind/internal/memkind_private.h"
#include "memkind/internal/pagesizes.h"
#include "memkind/internal/pool_allocator_internal_utils.h"

#include "assert.h"
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

// -------- private functions -------------------------------------------------

static FastSlabAllocator *fast_pool_allocator_get_slab(FastPoolAllocator *pool,
                                                       void *ptr)
{
    uintptr_t address = (uintptr_t)ptr;
    // TRACED_PAGESIZE is required to be a power of 2,
    // hence the optimization below is possible
    uintptr_t address_aligned = (address) & ~((uintptr_t)TRACED_PAGESIZE - 1);
    return fast_slab_tracker_get_fast_slab(pool->tracker, address_aligned);
}

// -------- public functions --------------------------------------------------

// TODO in the future, reducing code duplication with pool_allocator.h would be
// a good idea

MEMKIND_EXPORT void *
fast_pool_allocator_malloc_pages(FastPoolAllocator *pool, size_t size,
                                 const MmapCallback *user_mmap)
{
    if (size == 0)
        return NULL;
    size_t size_rank = size_to_rank_size(size);
    uint16_t hash = hasher_calculate_hash(size_rank);
    FastSlabAllocator *slab = pool->pool[hash];
    void *ret = NULL;
    // TODO get rid of the arrays below
    size_t dummy_address = 0ul;
    size_t dummy_nof_pages = 0ul;

    if (!slab) {
        FastSlabAllocator *null_slab = slab;
        slab = fast_slab_allocator_malloc(&pool->slabSlabAllocator);
        size_t slab_size = rank_size_to_size(size_rank);
        // TODO remove the "pages"
        int ret1 = fast_slab_allocator_init_pages(
            slab, slab_size, 0, &dummy_address, &dummy_nof_pages, user_mmap);
        if (ret1 != 0)
            return NULL;
        bool exchanged =
            atomic_compare_exchange_strong(&pool->pool[hash], &null_slab, slab);
        if (exchanged) {
            fast_slab_tracker_register(
                pool->tracker, (uintptr_t)slab->mappedMemory.area, slab);
        } else {
            fast_slab_allocator_destroy(slab);
            slab = atomic_load(&pool->pool[hash]);
        }
    }

    ret = fast_slab_allocator_malloc_pages(slab, &dummy_address,
                                           &dummy_nof_pages, user_mmap);

    return ret;
}

MEMKIND_EXPORT void *fast_pool_allocator_malloc(FastPoolAllocator *pool,
                                                size_t size)
{
    return fast_pool_allocator_malloc_pages(pool, size, &gStandardMmapCallback);
}

MEMKIND_EXPORT void *
fast_pool_allocator_realloc_pages(FastPoolAllocator *pool, void *ptr,
                                  size_t size, const MmapCallback *user_mmap)
{
    if (size == 0) {
        fast_pool_allocator_free(pool, ptr);
        return NULL;
    }
    if (ptr == NULL) {
        return fast_pool_allocator_malloc_pages(pool, size, user_mmap);
    }

    FastSlabAllocator *alloc = fast_pool_allocator_get_slab(pool, ptr);
    if (size_to_rank_size(size) == alloc->elementSize) {
        return ptr;
    }

    void *ret = fast_pool_allocator_malloc_pages(pool, size, user_mmap);
    if (ret) {
        size_t to_copy = size < alloc->elementSize ? size : alloc->elementSize;
        memcpy(ret, ptr, to_copy);
        // use alloc directly to avoid double lookup
        fast_slab_allocator_free(alloc, ptr);
    }

    return ret;
}

MEMKIND_EXPORT void fast_pool_allocator_free(FastPoolAllocator *pool, void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    FastSlabAllocator *alloc = fast_pool_allocator_get_slab(pool, ptr);
    assert(alloc && "allocator not registered!");
    fast_slab_allocator_free(alloc, ptr);
}

MEMKIND_EXPORT size_t fast_pool_allocator_usable_size(FastPoolAllocator *pool,
                                                      void *ptr)
{
    if (ptr == NULL)
        return 0;
    FastSlabAllocator *alloc = fast_pool_allocator_get_slab(pool, ptr);
    assert(alloc && "allocator not registered!");
    return alloc->elementSize;
}

MEMKIND_EXPORT int fast_pool_allocator_create(FastPoolAllocator *pool,
                                              const MmapCallback *user_mmap)
{
    uintptr_t addr = 0ul;
    size_t nof_pages = 0ul;
    int ret = fast_slab_allocator_init_pages(
        &pool->slabSlabAllocator, sizeof(FastSlabAllocator), UINT16_MAX, &addr,
        &nof_pages, user_mmap);
    if (ret == 0)
        (void)memset(pool->pool, 0, sizeof(pool->pool));
    fast_slab_tracker_create(&pool->tracker);

    return ret;
}

MEMKIND_EXPORT void fast_pool_allocator_destroy(FastPoolAllocator *pool)
{
    fast_slab_tracker_destroy(pool->tracker);
    for (size_t i = 0; i < UINT16_MAX; ++i)
        fast_slab_allocator_free(&pool->slabSlabAllocator, pool->pool[i]);
    fast_slab_allocator_destroy(&pool->slabSlabAllocator);
}
