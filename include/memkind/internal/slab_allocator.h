// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "config.h"
#include "memkind/internal/bigary.h"

#include "pthread.h"
#include "stddef.h"
#include "stdint.h"

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

#define USE_LOCKLESS

// -------- typedefs ----------------------------------------------------------

/// metadata
#ifdef USE_LOCKLESS
typedef _Atomic(struct freelist_node_meta *) freelist_node_thread_safe_meta_ptr;
#else
typedef struct freelist_node_meta *freelist_node_thread_safe_meta_ptr;
#endif

typedef struct freelist_node_meta {
    // pointer required to know how to free and which free lists to put it on
    struct slab_allocator *allocator;
    //     size_t size; not stored - all allocations have the same size
    struct freelist_node_meta *next;
} freelist_node_meta_t;

typedef struct glob_free_list {
    freelist_node_thread_safe_meta_ptr freelist;
    pthread_mutex_t mutex;
} glob_free_list_t;

typedef struct slab_allocator {
    glob_free_list_t globFreelist;
    bigary mappedMemory;
    size_t elementSize;
    // TODO stdatomic would poses issues to c++,
    // a wrapper might be necessary
    atomic_size_t used;
} SlabAllocator;

// -------- public functions --------------------------------------------------

/// @brief Initialize slab_allocator
/// @note only one initializer should be called!
extern int slab_allocator_init(SlabAllocator *alloc, size_t element_size,
                               size_t max_elements);
/// @brief Initialize slab_allocator and return mmapped pages
/// @note only one initializer should be called!
extern int slab_allocator_init_pages(SlabAllocator *alloc, size_t element_size,
                                     size_t max_elements, uintptr_t *addr,
                                     size_t *nof_pages,
                                     const MmapCallback *user_mmap);
extern void slab_allocator_destroy(SlabAllocator *alloc);
extern void *slab_allocator_malloc(SlabAllocator *alloc);
extern void *slab_allocator_malloc_pages(SlabAllocator *alloc,
                                         uintptr_t *page_start,
                                         size_t *nof_pages,
                                         const MmapCallback *user_mmap);
extern void slab_allocator_free(void *addr);
extern size_t slab_allocator_usable_size(void *addr);

#ifdef __cplusplus
}
#endif
