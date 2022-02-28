// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/mmap_tracing_queue.h"
#include "memkind/internal/memkind_private.h"

#include "assert.h"

MEMKIND_EXPORT void mmap_tracing_queue_create(MMapTracingQueue *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    slab_allocator_init(&queue->alloc, sizeof(MMapTracingNode), 0);
}

MEMKIND_EXPORT void mmap_tracing_queue_destroy(MMapTracingQueue *queue)
{
    assert(queue->head == NULL &&
           "cannot destroy a queue that contains elements");
    assert(queue->tail == NULL &&
           "cannot destroy a queue that contains elements");
    pthread_mutex_destroy(&queue->mutex);
    slab_allocator_destroy(&queue->alloc);
}

MEMKIND_EXPORT void
mmap_tracing_queue_multithreaded_push(MMapTracingQueue *queue,
                                      uintptr_t start_addr, size_t nof_pages)
{
    MMapTracingNode *node = slab_allocator_malloc(&queue->alloc);
    node->startAddr = start_addr;
    node->nofPages = nof_pages;
    node->next = NULL;
    pthread_mutex_lock(&queue->mutex);
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    pthread_mutex_unlock(&queue->mutex);
}

MEMKIND_EXPORT MMapTracingNode *
mmap_tracing_queue_multithreaded_take_all(MMapTracingQueue *queue)
{
    MMapTracingNode *ret;
    pthread_mutex_lock(&queue->mutex);
    ret = queue->head;
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_unlock(&queue->mutex);
    return ret;
}

/// @return true if processed, false if @p *head is null
MEMKIND_EXPORT bool mmap_tracing_queue_process_one(MMapTracingNode **head,
                                                   uintptr_t *start_addr,
                                                   size_t *nof_pages)
{
    bool processed = false;
    if (*head) {
        MMapTracingNode *temp = *head;
        *start_addr = temp->startAddr;
        *nof_pages = temp->nofPages;
        *head = temp->next;
        processed = true;
        slab_allocator_free(temp);
    }

    return processed;
}
