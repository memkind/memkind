// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind/internal/memkind_private.h>

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
} MTTAllocator;

void mtt_allocator_create(MTTAllocator *mtt_allocator);
int mtt_allocator_destroy(MTTAllocator *mtt_allocator);

#ifdef __cplusplus
}
#endif
