// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/multithreaded_touch_queue.h"
#include "memkind/internal/memkind_private.h"

#include "assert.h"
#include "stdatomic.h"

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#else

#define atomic_compare_exchange_weak(object, expected, desired)                \
    __atomic_compare_exchange((object), (expected), &(desired), true,          \
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define atomic_exchange(object, val)                                           \
    __atomic_exchange_n((object), (val), __ATOMIC_SEQ_CST)

#endif

MEMKIND_EXPORT void
multithreaded_touch_queue_create(MultithreadedTouchQueue *queue)
{
    queue->head = NULL;
    slab_allocator_init(&queue->alloc, sizeof(MultithreadedTouchNode), 0);
}

MEMKIND_EXPORT void
multithreaded_touch_queue_destroy(MultithreadedTouchQueue *queue)
{
    assert(queue->head == NULL &&
           "cannot destroy a queue that contains elements");
    slab_allocator_destroy(&queue->alloc);
}

MEMKIND_EXPORT void
multithreaded_touch_queue_multithreaded_push(MultithreadedTouchQueue *queue,
                                             uintptr_t address)
{
    AtomicMultithreadedTouchNodePtr node =
        (MultithreadedTouchNode *)slab_allocator_malloc(&queue->alloc);
    node->address = address;
    node->next = queue->head;
    while (!atomic_compare_exchange_weak(&queue->head, &node->next, node))
        ;
}

MEMKIND_EXPORT MultithreadedTouchNode *
multithreaded_touch_queue_multithreaded_take_all(MultithreadedTouchQueue *queue)
{
    AtomicMultithreadedTouchNodePtr node = NULL;
    return (MultithreadedTouchNode *)atomic_exchange(&queue->head, node);
}

MEMKIND_EXPORT bool
multithreaded_touch_queue_process_one(MultithreadedTouchNode **head,
                                      uintptr_t *address)
{
    bool processed = false;
    if (*head) {
        MultithreadedTouchNode *temp = *head;
        *address = temp->address;
        *head = temp->next;
        processed = true;
        slab_allocator_free(temp);
    }

    return processed;
}
