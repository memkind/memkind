// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "memkind/internal/mtt_internals.h" // TODO hide it from the exported headers
#include "memkind/internal/pebs.h" // TODO hide it from the exported headers

#include "pthread.h"

typedef enum
{
    THREAD_INIT,
    THREAD_AWAITING_FLUSH,
    THREAD_RUNNING,
    THREAD_FINISHED
} ThreadState_t;

typedef struct BackgroundThread {
    pthread_t bg_thread;
    ThreadState_t bg_thread_state;
    pthread_mutex_t bg_thread_cond_mutex;
    pthread_cond_t bg_thread_cond;
    PebsMetadata pebs;
} BackgroundThread;

typedef struct MTTAllocator {
    MttInternals internals;
    // TODO add support for sharing background threads
    BackgroundThread bgThread;
} MTTAllocator;

/// by default, all allocators share background thread
extern void mtt_allocator_create(MTTAllocator *mtt_allocator,
                                 const MTTInternalsLimits *limits);
extern void mtt_allocator_destroy(MTTAllocator *mtt_allocator);

extern void *mtt_allocator_malloc(MTTAllocator *mtt_allocator, size_t size);
extern void *mtt_allocator_realloc(MTTAllocator *mtt_allocator, void *ptr,
                                   size_t size);
extern void mtt_allocator_free(MTTAllocator *mtt_allocator, void *ptr);
extern size_t mtt_allocator_usable_size(MTTAllocator *mtt_allocator, void *ptr);

/// @brief Waits until all formerly mmapped pages become visible in the
/// rankings
///
/// This function awaits until background thread updates rankings; during this
/// operation, all mmapped pages are handled
///
/// @warning DO NOT destroy background thread/mtt_allocator while calling this
/// function
extern void mtt_allocator_await_flush(MTTAllocator *mtt_allocator);

#ifdef __cplusplus
}
#endif
