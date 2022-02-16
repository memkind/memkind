// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "memkind/internal/mtt_internals.h" // TODO hide it from the exported headers

#include "pthread.h"

typedef enum
{
    THREAD_INIT,
    THREAD_RUNNING,
    THREAD_FINISHED
} ThreadState_t;

typedef struct MTTAllocator {

    pthread_t bg_thread;
    ThreadState_t bg_thread_state;
    pthread_mutex_t bg_thread_cond_mutex;
    pthread_cond_t bg_thread_cond;

    MttInternals internals;
} MTTAllocator;

extern void mtt_allocator_create(MTTAllocator *mtt_allocator);
extern void mtt_allocator_destroy(MTTAllocator *mtt_allocator);

extern void *mtt_allocator_malloc(MTTAllocator *mtt_allocator, size_t size);
extern void *mtt_allocator_realloc(MTTAllocator *mtt_allocator, void *ptr,
                                   size_t size);
extern void mtt_allocator_free(void *ptr);

#ifdef __cplusplus
}
#endif
