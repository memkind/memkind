// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "memkind/internal/bigary.h"
#include "pthread.h"
#include "stddef.h"

#ifdef __cplusplus
#include <atomic>
// HACK
#define _Atomic(x) std::atomic<x>
#define atomic_size_t _Atomic(size_t)
extern "C" {
#else
#include "stdatomic.h"
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
    struct slab_alloc *allocator;
    //     size_t size; not stored - all allocations have the same size
    struct freelist_node_meta *next;
} freelist_node_meta_t;

typedef struct glob_free_list {
    freelist_node_thread_safe_meta_ptr freelist;
    pthread_mutex_t mutex;
} glob_free_list_t;

typedef struct slab_alloc {
    glob_free_list_t globFreelist;
    bigary mappedMemory;
    size_t elementSize;
    // TODO stdatomic would poses issues to c++,
    // a wrapper might be necessary
    atomic_size_t used;
} slab_alloc_t;

// -------- public functions --------------------------------------------------

extern int slab_alloc_init(slab_alloc_t *alloc, size_t element_size,
                           size_t max_elements);
extern void slab_alloc_destroy(slab_alloc_t *alloc);
extern void *slab_alloc_malloc(slab_alloc_t *alloc);
extern void slab_alloc_free(void *addr);

#ifdef __cplusplus
}
#endif
