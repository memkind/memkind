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

// -------- public functions --------------------------------------------------

// TODO in the future, reducing code duplication with pool_allocator.h would be
// a good idea

MEMKIND_EXPORT void *
fast_pool_allocator_malloc_pages(FastPoolAllocator *pool, size_t size,
                                 uintptr_t *address, size_t *nof_pages,
                                 const MmapCallback *user_mmap)
{
    if (size == 0)
        return NULL;
    size_t size_rank = size_to_rank_size(size);
    uint16_t hash = hasher_calculate_hash(size_rank);
    FastSlabAllocator *slab = pool->pool[hash];
    void *ret = NULL;
    size_t nof_pages_idx = 0ul;

    size_t mmap_idx = 0ul;
    if (!slab) {
        FastSlabAllocator *null_slab = slab;
        slab = fast_slab_allocator_malloc(&pool->slabSlabAllocator);
        size_t slab_size = rank_size_to_size(size_rank);
        nof_pages[nof_pages_idx] = 0ul;
        int ret1 = fast_slab_allocator_init_pages(
            slab, slab_size, 0, &address[nof_pages_idx],
            &nof_pages[nof_pages_idx], user_mmap);
        if (ret1 != 0)
            return NULL;
        bool exchanged =
            atomic_compare_exchange_strong(&pool->pool[hash], &null_slab, slab);
        if (exchanged) {
            uintptr_t page_start = address[nof_pages_idx];
            for (size_t i = 0ul; i < nof_pages[nof_pages_idx];
                 ++i, page_start += TRACED_PAGESIZE) {
                fast_slab_tracker_register(pool->tracker, page_start, slab);
            }
            ++nof_pages_idx;
        } else {
            fast_slab_allocator_destroy(slab);
            slab = atomic_load(&pool->pool[hash]);
        }
    }

    uintptr_t page_start = 0ul;
    nof_pages[nof_pages_idx] = 0ul;
    ret = fast_slab_allocator_malloc_pages(
        slab, &address[nof_pages_idx], &nof_pages[nof_pages_idx], user_mmap);

    page_start = address[nof_pages_idx];

    for (size_t i = 0ul; i < nof_pages[nof_pages_idx];
         ++i, page_start += TRACED_PAGESIZE) {
        fast_slab_tracker_register(pool->tracker, page_start, slab);
    }

    return ret;
}

MEMKIND_EXPORT void *fast_pool_allocator_malloc(FastPoolAllocator *pool,
                                                size_t size)
{
    uintptr_t dummy_address[2] = {0ul};
    size_t dummy_size[2] = {0ul};
    return fast_pool_allocator_malloc_pages(pool, size, dummy_address,
                                            dummy_size, &gStandardMmapCallback);
}

MEMKIND_EXPORT void *fast_pool_allocator_realloc(FastPoolAllocator *pool,
                                                 void *ptr, size_t size)
{
    fast_pool_allocator_free(pool, ptr);

    return fast_pool_allocator_malloc(pool, size);
}

MEMKIND_EXPORT void *fast_pool_allocator_realloc_pages(
    FastPoolAllocator *pool, void *ptr, size_t size, uintptr_t *addr,
    size_t *nof_pages, const MmapCallback *user_mmap)
{
    // FIXME this function does not conform to the standard
    // if allocation fails, previous allocation should not be freed
    fast_pool_allocator_free(pool, ptr);
    return fast_pool_allocator_malloc_pages(pool, size, addr, nof_pages,
                                            user_mmap);
}

MEMKIND_EXPORT void fast_pool_allocator_free(FastPoolAllocator *pool, void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    uintptr_t address = (uintptr_t)ptr;
    // TODO microoptimisation possible !
    uintptr_t address_aligned = (address / TRACED_PAGESIZE) * TRACED_PAGESIZE;
    FastSlabAllocator *alloc =
        fast_slab_tracker_get_fast_slab(pool->tracker, address_aligned);
    assert(alloc && "allocator not registered!");
    fast_slab_allocator_free(alloc, ptr);
}

MEMKIND_EXPORT size_t fast_pool_allocator_usable_size(FastPoolAllocator *pool,
                                                      void *ptr)
{
    if (ptr == NULL)
        return 0;

    uintptr_t address = (uintptr_t)ptr;
    // TODO microoptimisation possible !
    uintptr_t address_aligned = (address / TRACED_PAGESIZE) * TRACED_PAGESIZE;
    FastSlabAllocator *alloc =
        fast_slab_tracker_get_fast_slab(pool->tracker, address_aligned);
    assert(alloc && "allocator not registered!");
    return alloc->elementSize;
}

MEMKIND_EXPORT int fast_pool_allocator_create(FastPoolAllocator *pool,
                                              uintptr_t *addr,
                                              size_t *nof_pages,
                                              const MmapCallback *user_mmap)
{
    int ret = fast_slab_allocator_init_pages(
        &pool->slabSlabAllocator, sizeof(FastSlabAllocator), UINT16_MAX, addr,
        nof_pages, user_mmap);
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
