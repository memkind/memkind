// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/pool_allocator.h"
#include "memkind/internal/hasher.h"
#include "memkind/internal/memkind_private.h"

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

/// \note must be > 0
#define MIN_RANK_SIZE_POW_2 (4u) // 2^4 == 16u

// -------- static functions --------------------------------------------------

/// @brief Return the index of the most significant bit with value "1"
static uint64_t msb_set(uint64_t n)
{
    uint64_t idx;
    asm("bsr %1, %0" : "=r"(idx) : "r"(n));
    return idx;
}

/// @brief Return the index of the least significant bit with value "1"
static uint64_t lsb_set(uint64_t n)
{
    uint64_t idx;
    asm("bsf %1, %0" : "=r"(idx) : "r"(n));
    return idx;
}

/// @brief Convert @p size in bytes size rank
///
/// @return size_rank corresponding to @p size
///
/// @note this function is heavily optimised and might lack readability
///
/// @warning exact inverse calculation is not possible
/// @see size_rank_to_size for lossy inverse conversion
static size_t size_to_size_rank(size_t size)
{
    // find leftmost bit and rightmost bits
    uint64_t msb = msb_set(size);
    uint64_t lsb = lsb_set(size);

    // can be cleaned up/further optimised; good enough for POC
    if (msb == lsb) {
        bool msb_calculations = msb > MIN_RANK_SIZE_POW_2;
        uint64_t pow = msb - MIN_RANK_SIZE_POW_2;
        return msb_calculations * (pow << 1ul);
    } else {
        size_t msb_full = msb + 1ul;
        if (msb_full < MIN_RANK_SIZE_POW_2)
            return 0;
        bool mid_is_enough = (!(size & (1ul << (msb - 1ul))) ||
                              (lsb == msb - 1ul)); // second msb is set
        size_t ranks_to_add = !mid_is_enough; // 0 if is enough, 1 otherwise
        return ((msb - MIN_RANK_SIZE_POW_2) << 1ul) + 1ul + ranks_to_add;
    }
    // not reachable, all paths should have already returned
}

/// @brief Convert @p size_rank to absolute size in bytes
///
/// @return size, in bytes, corresponding to @p size_rank
///
/// @note this function is heavily optimised and might lack readability
/// @see size_to_size_rank
static size_t size_rank_to_size(size_t size_rank)
{
    // calculate hash without any conditional statements
    size_t min_pow2 = (size_rank >> 1ul) + MIN_RANK_SIZE_POW_2;
    size_t min_size = (1ul) << min_pow2;
    bool rank_is_pow_2 = (size_rank & 1ul) == 0ul;
    // cast bool to int directly - write optimised, branchless code
    size_t size_to_add_pow2 = !rank_is_pow_2; // rank is pow 2 -> nothing to add
    size_t size_to_add = size_to_add_pow2 << (min_pow2 - 1ul);
    size_t size = min_size + size_to_add;

    return size;
}

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
