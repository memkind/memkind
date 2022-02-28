// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "memkind/internal/slab_allocator.h"

#include "pthread.h"
#include "stdbool.h"
#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AllocationNode {
    uintptr_t startAddr;
    size_t nofPages;
    struct AllocationNode *next;
} MMapTracingNode;

typedef struct AllocationQueue {
    MMapTracingNode *head;
    MMapTracingNode *tail;
    pthread_mutex_t mutex;
    SlabAllocator alloc;
} MMapTracingQueue;

extern void mmap_tracing_queue_create(MMapTracingQueue *queue);
extern void mmap_tracing_queue_destroy(MMapTracingQueue *queue);

/// @brief Thread safe function for pushing map tracing info
extern void mmap_tracing_queue_multithreaded_push(MMapTracingQueue *queue,
                                                  uintptr_t start_addr,
                                                  size_t nof_pages);

/// @brief Thread safe function for clearing the queue and returning a list of
/// associated nodes
///
/// @note This function returns a list of all nodes, to be processed by the
/// mmap_tracing_queue_create function, for performance reasons - minimizing the
/// number and time of mutex locks
///
/// @note Ownership of the returned list is passed onto caller - it's their
/// responsibility to process it (free memory) and protect it from multithreaded
/// access (by multiple calls to mmap_tracing_queue_process_one)
///
/// @return list of nodes from the queue, to be processed by
/// mmap_tracing_queue_process_one
///
/// @warning the returned list has to be processed by
/// mmap_tracing_queue_process_one until the function returns false before
/// @p queue is destroyed !
extern MMapTracingNode *
mmap_tracing_queue_multithreaded_take_all(MMapTracingQueue *queue);

/// @brief Process single node of a list; return its contents by value and free
/// the allocated memory
///
/// @return true if processed, false if @p *head is null
///
/// @warning This function accesses the contents of the @p head list without a
/// mutex, as a part of optimisation!
extern bool mmap_tracing_queue_process_one(MMapTracingNode **head,
                                           uintptr_t *start_addr,
                                           size_t *nof_pages);

#ifdef __cplusplus
}
#endif
