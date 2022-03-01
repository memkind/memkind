// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "memkind/internal/slab_allocator.h"

#ifdef __cplusplus
#include <atomic>
// HACK
#define _Atomic(x) std::atomic<x>
#define atomic_size_t _Atomic(size_t)
extern "C" {
#else

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#else
#define atomic_size_t size_t
#define _Atomic(x) x
#endif
#endif

typedef _Atomic(struct fast_slab_allocator_freelist_node *)
    AtomicFastSlabAllocatorFreelistNodePtr;

typedef struct fast_slab_allocator_freelist_node {
    uintptr_t address;
    struct fast_slab_allocator_freelist_node *next;
} FastSlabAllocatorFreelistNode;

/// @brief Allocator for high performance access times
///
/// This allocator avoids keeping metadata per malloc/mmap in order to keep
/// the memory better aligned and guarantee faster memory access; free list
/// metadata is allocated separately, only when required
typedef struct fast_slab_allocator {
    AtomicFastSlabAllocatorFreelistNodePtr freeList;
    SlabAllocator freelistNodeAllocator;
    bigary mappedMemory;
    size_t elementSize;
    atomic_size_t used;

} FastSlabAllocator;

extern int fast_slab_allocator_init(FastSlabAllocator *alloc,
                                    size_t element_size, size_t max_elements);
extern int fast_slab_allocator_init_pages(FastSlabAllocator *alloc,
                                          size_t element_size,
                                          size_t max_elements, uintptr_t *addr,
                                          size_t *nof_pages);
extern void fast_slab_allocator_destroy(FastSlabAllocator *alloc);
extern void *fast_slab_allocator_malloc(FastSlabAllocator *alloc);
extern void *fast_slab_allocator_malloc_pages(FastSlabAllocator *alloc,
                                              uintptr_t *page_start,
                                              size_t *nof_pages);
extern void fast_slab_allocator_free(FastSlabAllocator *alloc, void *addr);

#ifdef __cplusplus
}
#endif
