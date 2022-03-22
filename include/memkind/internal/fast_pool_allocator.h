// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "config.h"
#include "memkind/internal/fast_slab_allocator.h"
#include "memkind/internal/slab_tracker.h"

#include "stddef.h"
#include "stdint.h"

#ifdef __cplusplus
#include <atomic>
// HACK
#define _Atomic(x) std::atomic<x>
#define atomic_fast_slab_alloc_ptr_t _Atomic(FastSlabAllocator *)
extern "C" {
#else

#if defined(HAVE_STDATOMIC_H) || true
#include <stdatomic.h>
#define MEMKIND_ATOMIC _Atomic
#else
#define MEMKIND_ATOMIC
#endif
#define atomic_fast_slab_alloc_ptr_t MEMKIND_ATOMIC(FastSlabAllocator *)
#endif

typedef struct FastPoolAllocator {
    atomic_fast_slab_alloc_ptr_t pool[UINT16_MAX];
    FastSlabAllocator slabSlabAllocator;
    SlabTracker *tracker;
} FastPoolAllocator;

/// @brief Initialize slab_allocator and return mmapped pages
/// @note only one initializer should be called!
extern int fast_pool_allocator_create(FastPoolAllocator *pool, uintptr_t *addr,
                                      size_t *nof_pages);
extern void fast_pool_allocator_destroy(FastPoolAllocator *pool);

extern void *fast_pool_allocator_malloc(FastPoolAllocator *pool, size_t size);
extern void *fast_pool_allocator_realloc_pages(FastPoolAllocator *pool,
                                               void *ptr, size_t size,
                                               uintptr_t *addr,
                                               size_t *nof_pages);
extern void *fast_pool_allocator_malloc_pages(FastPoolAllocator *pool,
                                              size_t size, uintptr_t *addr,
                                              size_t *nof_pages);
extern void *fast_pool_allocator_realloc(FastPoolAllocator *pool, void *ptr,
                                         size_t size);
extern void fast_pool_allocator_free(FastPoolAllocator *pool, void *ptr);
extern size_t fast_pool_allocator_usable_size(FastPoolAllocator *pool,
                                              void *ptr);

#ifdef __cplusplus
}
#endif
