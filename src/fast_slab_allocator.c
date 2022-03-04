// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/fast_slab_allocator.h"

#include "assert.h"
#include "pthread.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"

// -------- typedefs ----------------------------------------------------------

#ifndef MEMKIND_EXPORT
#define MEMKIND_EXPORT __attribute__((visibility("default")))
#endif

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#else

#define atomic_compare_exchange_weak(object, expected, desired)                \
    __atomic_compare_exchange((object), (expected), &(desired), true,          \
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define atomic_fetch_add_explicit __atomic_fetch_add
#define memory_order_relaxed      __ATOMIC_RELAXED
#endif

// -------- static functions --------------------------------------------------

static void fast_slab_alloc_glob_freelist_push_(FastSlabAllocator *alloc,
                                                void *addr)
{
    FastSlabAllocatorFreelistNode *node =
        slab_allocator_malloc(&alloc->freelistNodeAllocator);
    node->address = (uintptr_t)addr;
    do {
        node->next = alloc->freeList;
    } while (false ==
             atomic_compare_exchange_weak(&alloc->freeList, &node->next, node));
}

static void *fast_slab_alloc_glob_freelist_pop_(FastSlabAllocator *alloc)
{
    FastSlabAllocatorFreelistNode *node;
    do {
        node = alloc->freeList;
        if (!node)
            break;
    } while (false ==
             atomic_compare_exchange_weak(&alloc->freeList, &node, node->next));

    void *ret = node ? (void *)node->address : NULL;

    slab_allocator_free(node); // free on NULL is ok

    return ret;
}

static size_t slab_alloc_fetch_increment_used_(FastSlabAllocator *alloc)
{
    // the value is never atomically decreased, only thing we need is
    return atomic_fetch_add_explicit(&alloc->used, 1u, memory_order_relaxed);
}

static void *fast_slab_alloc_malloc_pages_(FastSlabAllocator *alloc,
                                           uintptr_t *page_start,
                                           size_t *nof_pages)
{
    size_t free_idx = slab_alloc_fetch_increment_used_(alloc);
    size_t offset = alloc->elementSize * free_idx;

    // TODO handle failure gracefully instead of die - in biary_alloc
    bigary_alloc_pages(&alloc->mappedMemory,
                       (free_idx + 1) * alloc->elementSize, page_start,
                       nof_pages);

    return ((uint8_t *)alloc->mappedMemory.area) + offset;
}

static void *fast_slab_alloc_malloc_(FastSlabAllocator *alloc)
{
    size_t free_idx = slab_alloc_fetch_increment_used_(alloc);
    size_t offset = alloc->elementSize * free_idx;

    // TODO handle failure gracefully instead of die - in biary_alloc
    bigary_alloc(&alloc->mappedMemory, (free_idx + 1) * alloc->elementSize);

    return ((uint8_t *)alloc->mappedMemory.area) + offset;
}

// -------- public functions --------------------------------------------------

MEMKIND_EXPORT int fast_slab_allocator_init(FastSlabAllocator *alloc,
                                            size_t element_size,
                                            size_t max_elements)
{
    // TODO handle failure gracefully instead of die - in biary_alloc
    alloc->elementSize = element_size;
    size_t max_elements_size = max_elements * alloc->elementSize;
    bigary_init(&alloc->mappedMemory, BIGARY_DRAM, max_elements_size);
    alloc->used = 0u;

    int ret = slab_allocator_init(&alloc->freelistNodeAllocator,
                                  sizeof(FastSlabAllocatorFreelistNode), 0);
    alloc->freeList = NULL;

    return ret;
}

MEMKIND_EXPORT int fast_slab_allocator_init_pages(FastSlabAllocator *alloc,
                                                  size_t element_size,
                                                  size_t max_elements,
                                                  uintptr_t *addr,
                                                  size_t *nof_pages)
{
    // TODO handle failure gracefully instead of die - in biary_alloc
    alloc->elementSize = element_size;
    size_t max_elements_size = max_elements * alloc->elementSize;
    bigary_init_pages(&alloc->mappedMemory, BIGARY_DRAM, max_elements_size,
                      addr, nof_pages);
    alloc->used = 0u;

    int ret = slab_allocator_init(&alloc->freelistNodeAllocator,
                                  sizeof(FastSlabAllocatorFreelistNode), 0);
    alloc->freeList = NULL;

    return ret;
}

MEMKIND_EXPORT void fast_slab_allocator_destroy(FastSlabAllocator *alloc)
{
    bigary_destroy(&alloc->mappedMemory);
    slab_allocator_destroy(&alloc->freelistNodeAllocator);
}

MEMKIND_EXPORT void *fast_slab_allocator_malloc(FastSlabAllocator *alloc)
{
    void *ret = fast_slab_alloc_glob_freelist_pop_(alloc);
    if (!ret) {
        ret = fast_slab_alloc_malloc_(alloc);
    }

    return ret;
}

MEMKIND_EXPORT void *fast_slab_allocator_malloc_pages(FastSlabAllocator *alloc,
                                                      uintptr_t *page_start,
                                                      size_t *nof_pages)
{
    void *ret = fast_slab_alloc_glob_freelist_pop_(alloc);
    if (!ret) {
        ret = fast_slab_alloc_malloc_pages_(alloc, page_start, nof_pages);
    }

    return ret;
}

MEMKIND_EXPORT void fast_slab_allocator_free(FastSlabAllocator *alloc,
                                             void *addr)
{
    if (addr)
        fast_slab_alloc_glob_freelist_push_(alloc, addr);
}
