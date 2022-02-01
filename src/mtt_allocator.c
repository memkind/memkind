// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/memkind_log.h>
#include <mtt_allocator.h>

#include <stdlib.h>

// Function required for pthread_create()
// TODO: move it to a separate module and add an implementation
static void *mtt_run_bg_thread(void *thread_state)
{
    ThreadState_t *bg_thread_state = thread_state;

    pthread_mutex_lock(&mtt_bg_thread_cond_mutex);
    *bg_thread_state = THREAD_RUNNING;
    pthread_cond_signal(&mtt_bg_thread_cond);
    pthread_mutex_unlock(&mtt_bg_thread_cond_mutex);

    while (1) {
        if (*bg_thread_state == THREAD_FINISHED) {
            pthread_exit(NULL);
        }
    }
}

//--------------------------------------------------------------------------
// ----------------------------- Public API --------------------------------
//--------------------------------------------------------------------------

MEMKIND_EXPORT void mtt_allocator_create(MTTAllocator *mtt_allocator)
{
    mtt_allocator->bg_thread_state = THREAD_INIT;
    int ret = pthread_create(&mtt_allocator->bg_thread, NULL, mtt_run_bg_thread,
                             (void *)&mtt_allocator->bg_thread_state);
    if (ret) {
        log_fatal("Creating MTT allocator background thread failed!");
        abort();
    }
    log_info("MTT background thread created");
}

MEMKIND_EXPORT int mtt_allocator_destroy(MTTAllocator *mtt_allocator)
{
    pthread_mutex_lock(&mtt_bg_thread_cond_mutex);
    while (mtt_allocator->bg_thread_state == THREAD_INIT) {
        pthread_cond_wait(&mtt_bg_thread_cond, &mtt_bg_thread_cond_mutex);
    }
    pthread_mutex_unlock(&mtt_bg_thread_cond_mutex);

    mtt_allocator->bg_thread_state = THREAD_FINISHED;
    pthread_join(mtt_allocator->bg_thread, NULL);
    log_info("MTT background thread closed");

    return 0;
}
