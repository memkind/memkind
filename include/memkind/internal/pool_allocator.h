// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "config.h"
#include "memkind/internal/slab_allocator.h"

#include "stddef.h"
#include "stdint.h"

#ifdef __cplusplus
#include <atomic>
// HACK
#define _Atomic(x) std::atomic<x>
#define atomic_slab_alloc_ptr_t _Atomic(SlabAllocator *)
extern "C" {
#else

#if defined(HAVE_STDATOMIC_H) || true
#include <stdatomic.h>
#define MEMKIND_ATOMIC _Atomic
#else
#define MEMKIND_ATOMIC
#endif
#define atomic_slab_alloc_ptr_t MEMKIND_ATOMIC(SlabAllocator *)
#endif

typedef struct PoolAllocator {
    atomic_slab_alloc_ptr_t pool[UINT16_MAX];
    SlabAllocator slabSlabAllocator;
} PoolAllocator;

extern int pool_allocator_create(PoolAllocator *pool);
extern void pool_allocator_destroy(PoolAllocator *pool);

extern void *pool_allocator_malloc(PoolAllocator *pool, size_t size);
extern void *pool_allocator_realloc_pages(PoolAllocator *pool, void *ptr,
                                          size_t size, uintptr_t *addr,
                                          size_t *nof_pages);
extern void *pool_allocator_malloc_pages(PoolAllocator *pool, size_t size,
                                         uintptr_t *addr, size_t *nof_pages);
extern void *pool_allocator_realloc(PoolAllocator *pool, void *ptr,
                                    size_t size);
extern void pool_allocator_free(void *ptr);

#ifdef __cplusplus
}
#endif
