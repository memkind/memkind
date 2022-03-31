// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/pebs.h>

#include <mtt_allocator.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#define BG_THREAD_FREQUENCY 2.0

// typedefs -------------------------------------------------------------------

typedef struct MTTAllocatorNode {
    MTTAllocator *allocator;
    struct MTTAllocatorNode *next;
} MTTAllocatorNode;

// global variables -----------------------------------------------------------

// TODO decide about singleton, decisions need to be made during productization
static MTTAllocatorNode *root = NULL;
static SlabAllocator nodeSlab;
static pthread_mutex_t allocatorsMutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t last_timestamp;

// static function declarations -----------------------------------------------

static void *mtt_run_bg_thread(void *mtt_alloc);
/// @pre allocatorsMutex must be acquired before calling
static void touch_callback(uintptr_t address, uint64_t timestamp);
/// @pre allocatorsMutex must be acquired before calling
static void update_rankings();
static void register_mtt_allocator(MTTAllocator *mtt_allocator);
static void unregister_mtt_allocator(MTTAllocator *mtt_allocator);

static void timespec_add(struct timespec *modified,
                         const struct timespec *toadd);
static struct timespec timespec_diff(const struct timespec *time1,
                                     const struct timespec *time0);
static void timespec_millis_to_timespec(double millis, struct timespec *out);
static void timespec_get_time(struct timespec *time);

// static function definitions ------------------------------------------------

// Function required for pthread_create()
// TODO: move it to a separate module and add an implementation
// TODO handle touches and ranking updates from this branch
static void *mtt_run_bg_thread(void *bg_thread_ptr)
{
    BackgroundThread *bg_thread = bg_thread_ptr;
    pthread_mutex_t *bg_thread_cond_mutex = &bg_thread->bg_thread_cond_mutex;

    struct timespec start_time, end_time;
    // calculate time period to wait
    double bg_freq_hz = BG_THREAD_FREQUENCY;
    double period_ms = 1000 / bg_freq_hz;
    struct timespec tv_period;
    timespec_millis_to_timespec(period_ms, &tv_period);

    // set low priority
    int policy;
    struct sched_param param;
    pthread_getschedparam(pthread_self(), &policy, &param);
    param.sched_priority = sched_get_priority_min(policy);
    pthread_setschedparam(pthread_self(), policy, &param);

    pthread_mutex_lock(bg_thread_cond_mutex);
    bg_thread->bg_thread_state = THREAD_RUNNING;
    pthread_cond_broadcast(&bg_thread->bg_thread_cond);
    pthread_mutex_unlock(bg_thread_cond_mutex);

    bool run = true;
    while (run) {
        bool flushing_rankings = false;

        pthread_mutex_lock(bg_thread_cond_mutex);
        if (bg_thread->bg_thread_state == THREAD_FINISHED) {
            run = false;
        } else if (bg_thread->bg_thread_state == THREAD_AWAITING_FLUSH) {
            // we need to inform the awaiting function that we processed
            // rankings, right after update_rankings() function call returns
            flushing_rankings = true;
        }
        pthread_mutex_unlock(bg_thread_cond_mutex);

        // calculate time to wait
        timespec_get_time(&start_time);
        timespec_add(&start_time, &tv_period);

        pthread_mutex_lock(&allocatorsMutex);
        pebs_monitor(&bg_thread->pebs);
        update_rankings();
        pthread_mutex_unlock(&allocatorsMutex);

        if (flushing_rankings) {
            // inform all awaiting threads - reset bg_thread_state
            pthread_mutex_lock(bg_thread_cond_mutex);
            bg_thread->bg_thread_state = THREAD_RUNNING;
            // broadcast bg_thread_state change to all awaiting threads
            pthread_cond_broadcast(&bg_thread->bg_thread_cond);
            pthread_mutex_unlock(bg_thread_cond_mutex);
        }

        timespec_get_time(&end_time);
        struct timespec wait_time = timespec_diff(&start_time, &end_time);
        if ((wait_time.tv_sec > 0) ||
            ((wait_time.tv_sec == 0) && (wait_time.tv_nsec > 0))) {
            pthread_cond_clockwait(&bg_thread->bg_thread_cond,
                                   &bg_thread->bg_thread_cond_mutex,
                                   CLOCK_MONOTONIC, &wait_time);
        }
    }

    return NULL;
}

static void touch_callback(uintptr_t address, uint64_t timestamp)
{
    MTTAllocatorNode *cnode = root;
    while (cnode) {
        mtt_internals_touch(&cnode->allocator->internals, address);
        cnode = cnode->next;
    }
    last_timestamp = timestamp;
}

static void update_rankings()
{
    MTTAllocatorNode *cnode = root;
    while (cnode) {
        mtt_internals_ranking_update(&cnode->allocator->internals,
                                     last_timestamp);
        cnode = cnode->next;
    }
}

static void register_mtt_allocator(MTTAllocator *mtt_allocator)
{
    pthread_mutex_lock(&allocatorsMutex);
    if (root) {
        MTTAllocatorNode *nnode = slab_allocator_malloc(&nodeSlab);
        nnode->allocator = mtt_allocator;
        nnode->next = root;
        root = nnode;
    } else {
        slab_allocator_init(&nodeSlab, sizeof(MTTAllocatorNode), 10);
        root = slab_allocator_malloc(&nodeSlab);
        root->allocator = mtt_allocator;
        root->next = NULL;
    }
    pthread_mutex_unlock(&allocatorsMutex);
}

static void unregister_mtt_allocator(MTTAllocator *mtt_allocator)
{
    pthread_mutex_lock(&allocatorsMutex);
    MTTAllocatorNode *cnode = root;
    MTTAllocatorNode *prev_node = root;
    bool single_allocator_registered = !(bool)root->next;
    while (cnode->allocator != mtt_allocator) {
        prev_node = cnode;
        cnode = cnode->next;
    }
    // remove cnode from linked list
    prev_node->next = cnode->next;
    slab_allocator_free(cnode);
    if (single_allocator_registered) {
        // no registered allocators
        slab_allocator_destroy(&nodeSlab);
        root = NULL;
    }
    pthread_mutex_unlock(&allocatorsMutex);
}

void background_thread_init(MTTAllocator *mtt_allocator)
{
    BackgroundThread *bg_thread = &mtt_allocator->bgThread;

    pthread_mutex_init(&bg_thread->bg_thread_cond_mutex, NULL);
    pthread_cond_init(&bg_thread->bg_thread_cond, NULL);
    pebs_create(mtt_allocator, &bg_thread->pebs, touch_callback);

    bg_thread->bg_thread_state = THREAD_INIT;
    int ret = pthread_create(&bg_thread->bg_thread, NULL, mtt_run_bg_thread,
                             (void *)bg_thread);
    if (ret) {
        log_fatal("Creating MTT allocator background thread failed!");
        abort();
    }
    pthread_mutex_lock(&bg_thread->bg_thread_cond_mutex);
    while (bg_thread->bg_thread_state == THREAD_INIT) {
        pthread_cond_wait(&bg_thread->bg_thread_cond,
                          &bg_thread->bg_thread_cond_mutex);
    }
    pthread_mutex_unlock(&bg_thread->bg_thread_cond_mutex);
}

void background_thread_fini(MTTAllocator *mtt_allocator)
{
    BackgroundThread *bg_thread = &mtt_allocator->bgThread;

    pthread_mutex_t *bg_thread_cond_mutex = &bg_thread->bg_thread_cond_mutex;
    pthread_mutex_lock(bg_thread_cond_mutex);
    while (bg_thread->bg_thread_state == THREAD_INIT) {
        pthread_cond_wait(&bg_thread->bg_thread_cond, bg_thread_cond_mutex);
    }
    bg_thread->bg_thread_state = THREAD_FINISHED;
    pthread_mutex_unlock(bg_thread_cond_mutex);

    pthread_join(bg_thread->bg_thread, NULL);
    log_info("MTT background thread closed");

    pebs_destroy(mtt_allocator, &bg_thread->pebs);
    pthread_mutex_destroy(bg_thread_cond_mutex);
    pthread_cond_destroy(&bg_thread->bg_thread_cond);
}

static void timespec_add(struct timespec *modified,
                         const struct timespec *toadd)
{
    const uint64_t S_TO_NS = 1e9;
    modified->tv_sec += toadd->tv_sec;
    modified->tv_nsec += toadd->tv_nsec;
    modified->tv_sec += (modified->tv_nsec / S_TO_NS);
    modified->tv_nsec %= S_TO_NS;
}

static struct timespec timespec_diff(const struct timespec *time1,
                                     const struct timespec *time0)
{
    struct timespec diff;

    assert(time1 && time1->tv_nsec < 1000000000);
    assert(time0 && time0->tv_nsec < 1000000000);
    diff.tv_sec = time1->tv_sec - time0->tv_sec;
    diff.tv_nsec = time1->tv_nsec - time0->tv_nsec;

    if (diff.tv_nsec < 0) {
        diff.tv_nsec += 1000000000;
        diff.tv_sec--;
    }

    return diff;
}

static void timespec_get_time(struct timespec *time)
{
    // calculate time to wait
    int ret = clock_gettime(CLOCK_MONOTONIC, time);
    if (ret != 0) {
        log_fatal("ASSERT PEBS Monitor clock_gettime() FAILURE: %s",
                  strerror(errno));
        exit(-1);
    }
}

static void timespec_millis_to_timespec(double millis, struct timespec *out)
{
    out->tv_sec = (uint64_t)(millis / 1e3);
    double ms_to_convert = millis - out->tv_sec * 1000ul;
    out->tv_nsec = (uint64_t)(ms_to_convert * 1e6);
}

//--------------------------------------------------------------------------
// ----------------------------- Public API --------------------------------
//--------------------------------------------------------------------------

MEMKIND_EXPORT void mtt_allocator_create(MTTAllocator *mtt_allocator,
                                         const MTTInternalsLimits *limits)
{
    // TODO get timestamp from somewhere!!! (bg thread? PEBS?)
    uint64_t timestamp = 0;
    mtt_internals_create(&mtt_allocator->internals, timestamp, limits);
    register_mtt_allocator(mtt_allocator);
    background_thread_init(mtt_allocator);
    log_info("MTT background thread created");
}

MEMKIND_EXPORT void mtt_allocator_destroy(MTTAllocator *mtt_allocator)
{
    // TODO registering should not be global, but per bg thread
    background_thread_fini(mtt_allocator);
    unregister_mtt_allocator(mtt_allocator);
    mtt_internals_destroy(&mtt_allocator->internals);
}

MEMKIND_EXPORT void *mtt_allocator_malloc(MTTAllocator *mtt_allocator,
                                          size_t size)
{
    return mtt_internals_malloc(&mtt_allocator->internals, size);
}

MEMKIND_EXPORT void *mtt_allocator_realloc(MTTAllocator *mtt_allocator,
                                           void *ptr, size_t size)
{
    return mtt_internals_realloc(&mtt_allocator->internals, ptr, size);
}

MEMKIND_EXPORT void mtt_allocator_free(MTTAllocator *mtt_allocator, void *ptr)
{
    mtt_internals_free(&mtt_allocator->internals, ptr);
}

MEMKIND_EXPORT size_t mtt_allocator_usable_size(MTTAllocator *mtt_allocator,
                                                void *ptr)
{
    return mtt_internals_usable_size(&mtt_allocator->internals, ptr);
}

MEMKIND_EXPORT void mtt_allocator_await_flush(MTTAllocator *mtt_allocator)
{
    pthread_mutex_lock(&mtt_allocator->bgThread.bg_thread_cond_mutex);
    mtt_allocator->bgThread.bg_thread_state = THREAD_AWAITING_FLUSH;
    // broadcast bg_thread_state update
    pthread_cond_broadcast(&mtt_allocator->bgThread.bg_thread_cond);
    while (mtt_allocator->bgThread.bg_thread_state == THREAD_AWAITING_FLUSH) {
        pthread_cond_wait(&mtt_allocator->bgThread.bg_thread_cond,
                          &mtt_allocator->bgThread.bg_thread_cond_mutex);
    }
    pthread_mutex_unlock(&mtt_allocator->bgThread.bg_thread_cond_mutex);
}
