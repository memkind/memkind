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

/// @brief Initialize slab_allocator
/// @note only one initializer should be called!
extern int pool_allocator_create(PoolAllocator *pool,
                                 const MmapCallback *user_mmap);
extern void pool_allocator_destroy(PoolAllocator *pool,
                                   const MmapCallback *user_mmap);

extern void *pool_allocator_malloc(PoolAllocator *pool, size_t size);
extern void *pool_allocator_malloc_mmap(PoolAllocator *pool, size_t size,
                                        const MmapCallback *user_mmap);
extern void *pool_allocator_realloc(PoolAllocator *pool, void *ptr,
                                    size_t size);
extern void *pool_allocator_realloc_mmap(PoolAllocator *pool, void *ptr,
                                         size_t size,
                                         const MmapCallback *user_mmap);
extern void pool_allocator_free(PoolAllocator *pool, void *ptr);
extern size_t pool_allocator_usable_size(PoolAllocator *pool, void *ptr);

#ifdef __cplusplus
}
#endif
