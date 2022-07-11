// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "memkind/internal/slab_allocator.h"

#include "pthread.h"
#include "stdbool.h"
#include "stddef.h"

#ifdef __cplusplus
#include <atomic>
// HACK
#define _Atomic(x) std::atomic<x>
extern "C" {
#else
#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#else
#define _Atomic(x) x
#endif
#endif

typedef _Atomic(struct MultithreadedTouchNode *)
    AtomicMultithreadedTouchNodePtr;

typedef struct MultithreadedTouchNode {
    uintptr_t address;
    struct MultithreadedTouchNode *next;
} MultithreadedTouchNode;

typedef struct MultithreadedTouchQueue {
    AtomicMultithreadedTouchNodePtr head;
    SlabAllocator alloc;
} MultithreadedTouchQueue;

extern void multithreaded_touch_queue_create(MultithreadedTouchQueue *queue);
extern void multithreaded_touch_queue_destroy(MultithreadedTouchQueue *queue);

/// @brief Thread safe function for pushing map tracing info
extern void
multithreaded_touch_queue_multithreaded_push(MultithreadedTouchQueue *queue,
                                             uintptr_t address);

extern MultithreadedTouchNode *multithreaded_touch_queue_multithreaded_take_all(
    MultithreadedTouchQueue *queue);

/// @brief Process single node of a list; return its contents by value and free
/// the allocated memory
///
/// @return true if processed, false if @p *head is null
///
/// @warning This function accesses the contents of the @p head list without a
/// mutex, as a part of optimisation!
extern bool multithreaded_touch_queue_process_one(MultithreadedTouchNode **head,
                                                  uintptr_t *address);

#ifdef __cplusplus
}
#endif
