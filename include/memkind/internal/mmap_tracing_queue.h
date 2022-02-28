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
extern void mmap_tracing_queue_multithreaded_push(MMapTracingQueue *queue,
                                                  uintptr_t start_addr,
                                                  size_t nof_pages);
extern MMapTracingNode *
mmap_tracing_queue_multithreaded_take_all(MMapTracingQueue *queue);

extern bool mmap_tracing_queue_process_one(MMapTracingNode **head,
                                           uintptr_t *start_addr,
                                           size_t *nof_pages);

#ifdef __cplusplus
}
#endif
