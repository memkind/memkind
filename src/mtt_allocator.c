// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/pagesizes.h>
#include <memkind/internal/pebs.h>
#include <memkind/internal/ranking_utils.h>
#include <memkind/internal/timespec_ops.h>

#include <mtt_allocator.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>

#define BG_THREAD_FREQUENCY 0.1

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
static pthread_once_t mtt_register_atfork_once = PTHREAD_ONCE_INIT;
static pthread_once_t nodemasks_init_once = PTHREAD_ONCE_INIT;

// static function declarations -----------------------------------------------

static void *mtt_run_bg_thread(void *mtt_alloc);
/// @pre allocatorsMutex must be acquired before calling
static void touch_callback(uintptr_t address, uint64_t timestamp);
/// @pre allocatorsMutex must be acquired before calling
static void update_rankings(atomic_bool *interrupt);
static void balance_rankings();
static void register_mtt_allocator(MTTAllocator *mtt_allocator);
static void unregister_mtt_allocator(MTTAllocator *mtt_allocator);
static void *mtt_allocator_mmap_cb_wrapper(void *arg, void *addr, size_t len,
                                           int prot, int flags, int fd,
                                           __off_t offset);
static void background_thread_start(BackgroundThread *bg_thread);
static void background_thread_stop(MTTAllocator *mtt_allocator);

// static function definitions ------------------------------------------------

/// @brief Wrapper for mmap with PMEM/DRAM balancing
///
/// @note At the function exit, we should have at most hardLimit data in DRAM
static void *mtt_allocator_mmap_cb_wrapper(void *arg, void *addr, size_t len,
                                           int prot, int flags, int fd,
                                           __off_t offset)
{
    return mtt_allocator_mmap((MTTAllocator *)arg, addr, len, prot, flags, fd,
                              offset);
}

/// @brief Wrapper for mmap
///
/// @pre @p arg (MTTAllocator*) should have the following fields initialized:
///     - allocator->internals.usedDram,
///     - allocator->internals.limits.hardLimit
static void *mtt_allocator_mmap_init_once(void *arg, void *addr, size_t len,
                                          int prot, int flags, int fd,
                                          __off_t offset)
{
    MTTAllocator *allocator = arg;
    // TODO assert that the same area is not mmapped twice !!!
    // TODO assert that the allocator is initialized
    // TODO assert that the allocator is not called recursively
    size_t prev_used_dram =
        atomic_fetch_add(&allocator->internals.usedDram, len);

    assert(prev_used_dram == 0ul &&
           "This wrapper is only supposed to be used once, at init time!");
    assert(len < allocator->internals.limits.hardLimit &&
           "Balancing not available at init time, increase hard limit!");
    void *ret = mmap(addr, len, prot, flags, fd, offset);
    if (ret) // TODO make sure nothing is broken during final refactoring!
        mtt_internals_tracing_multithreaded_push(
            &allocator->internals, (uintptr_t)ret, len / TRACED_PAGESIZE);
    return ret;
}

// Function required for pthread_create()
// TODO: move it to a separate module and add an implementation
// TODO handle touches and ranking updates from this branch
static void *mtt_run_bg_thread(void *bg_thread_ptr)
{
    BackgroundThread *bg_thread = bg_thread_ptr;
    pthread_mutex_t *bg_thread_cond_mutex = &bg_thread->bg_thread_cond_mutex;

    struct timespec end_time;
    // calculate time period to wait
    double period_ms = 1000 / BG_THREAD_FREQUENCY;
    struct timespec tv_period;
    timespec_millis_to_timespec(period_ms, &tv_period);

    // set low priority
    int policy;
    struct sched_param param;
    pthread_getschedparam(pthread_self(), &policy, &param);
    param.sched_priority = sched_get_priority_min(policy);
    pthread_setschedparam(pthread_self(), policy, &param);

    // no sleep on first iteration
    // TODO: we use CLOCK_REALTIME here only because pthread_cond_timedwait
    // waits using this clock. CLOCK_MONOTONIC would be better here, but not
    // all Linux distros implements pthread_cond_clockwait that could wait on
    // arbitrary type of CLOCK
    timespec_get_time(CLOCK_REALTIME, &end_time);

    pthread_mutex_lock(bg_thread_cond_mutex);
    bg_thread->bg_thread_state = THREAD_RUNNING;
    pthread_cond_broadcast(&bg_thread->bg_thread_cond);
    pthread_mutex_unlock(bg_thread_cond_mutex);

    ThreadState_t bg_thread_state = THREAD_RUNNING;

    while (bg_thread_state != THREAD_FINISHED) {
        pthread_mutex_lock(bg_thread_cond_mutex);
        // reset the "interrupt" flag
        atomic_store(&bg_thread->interrupt, false);
        bg_thread_state = bg_thread->bg_thread_state;
        // perform sleep only if no sleep interrupt was scheduled
        if (bg_thread_state == THREAD_RUNNING) {
            // wait until next occurrence (according to frequency) or interrupt
            pthread_cond_timedwait(&bg_thread->bg_thread_cond,
                                   &bg_thread->bg_thread_cond_mutex, &end_time);
        }
        pthread_mutex_unlock(bg_thread_cond_mutex);

        // calculate time to wait - only what remains after current iteration
        timespec_get_time(CLOCK_REALTIME, &end_time);
        timespec_add(&end_time, &tv_period);

        pthread_mutex_lock(&allocatorsMutex);
        switch (bg_thread_state) {
            case THREAD_AWAITING_BALANCE:
                // perform the minimal operations that are necessary to balance
                // DRAM-PMEM ratio
                balance_rankings();
                break;
            case THREAD_RUNNING:
            case THREAD_AWAITING_FLUSH:
                pebs_monitor(&bg_thread->pebs);
                update_rankings(&bg_thread->interrupt);
                break;
            case THREAD_FINISHED:
                // do nothing
                break;
            case THREAD_INIT:
                assert(false && "state should never occur");
                break;
        }
        pthread_mutex_unlock(&allocatorsMutex);
        if (bg_thread_state == THREAD_AWAITING_BALANCE ||
            bg_thread_state == THREAD_AWAITING_FLUSH) {
            // inform all awaiting threads - reset bg_thread_state
            pthread_mutex_lock(bg_thread_cond_mutex);
            bg_thread->bg_thread_state = THREAD_RUNNING;
            // broadcast bg_thread_state change to all awaiting threads
            pthread_cond_broadcast(&bg_thread->bg_thread_cond);
            pthread_mutex_unlock(bg_thread_cond_mutex);
        }
    }
    pthread_mutex_lock(&allocatorsMutex);
    update_rankings(&bg_thread->interrupt); // flush mmapTracingQueue
    pthread_mutex_unlock(&allocatorsMutex);

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

static void update_rankings(atomic_bool *interrupt)
{
    MTTAllocatorNode *cnode = root;
    while (cnode) {
        mtt_internals_ranking_update(
            &cnode->allocator->internals, last_timestamp,
            &cnode->allocator->internals.usedDram, interrupt);
        cnode = cnode->next;
    }
}

static void balance_rankings()
{
    MTTAllocatorNode *cnode = root;
    while (cnode) {
        size_t surplus = mtt_internals_ranking_balance(
            &cnode->allocator->internals, &cnode->allocator->internals.usedDram,
            cnode->allocator->internals.limits.hardLimit);
        // TODO implement handling for the case that follows (direct allocation
        // to PMEM)
        assert(
            surplus == 0lu &&
            "Not able to balance allocations down to hardLimit! Case not handled, some mappings should probably go directly to pmem");
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

    background_thread_start(bg_thread);
}

static void background_thread_start(BackgroundThread *bg_thread)
{
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

static void background_thread_stop(MTTAllocator *mtt_allocator)
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
    assert(mtt_allocator->internals.mmapTracingQueue.head == NULL);
    log_info("MTT background thread closed");
}

void background_thread_fini(MTTAllocator *mtt_allocator)
{
    BackgroundThread *bg_thread = &mtt_allocator->bgThread;

    background_thread_stop(mtt_allocator);

    pebs_destroy(mtt_allocator, &bg_thread->pebs);
    pthread_mutex_destroy(&bg_thread->bg_thread_cond_mutex);
    pthread_cond_destroy(&bg_thread->bg_thread_cond);
}

static void mtt_prefork(void)
{
    MTTAllocatorNode *cnode = root;
    while (cnode) {
        background_thread_stop(cnode->allocator);
        pebs_disable(cnode->allocator, &cnode->allocator->bgThread.pebs);
        cnode = cnode->next;
    }
}

static void mtt_postfork(void)
{
    MTTAllocatorNode *cnode = root;
    while (cnode) {
        pebs_enable(cnode->allocator, &cnode->allocator->bgThread.pebs);
        background_thread_start(&cnode->allocator->bgThread);
        cnode = cnode->next;
    }
}

static void mtt_register_atfork(void)
{
    pthread_atfork(mtt_prefork, mtt_postfork, mtt_postfork);
}

//--------------------------------------------------------------------------
// ----------------------------- Public API --------------------------------
//--------------------------------------------------------------------------

MEMKIND_EXPORT void mtt_allocator_create(MTTAllocator *mtt_allocator,
                                         const MTTInternalsLimits *limits)
{
    pthread_once(&mtt_register_atfork_once, mtt_register_atfork);
    pthread_once(&nodemasks_init_once, init_nodemasks);

    // TODO get timestamp from somewhere!!! (bg thread? PEBS?)
    uint64_t timestamp = 0;
    // TODO this is really super problematic... allocator requires background
    // thread and background thread requires allocator
    // TODO this does not work!!!! mtt_allocator_init_mmap requires
    // mtt_allocator->internals !!!
    // TODO add another version, with hard limit only for internals construction
    MmapCallback mmap_callback;
    mmap_callback.arg = mtt_allocator;
    mmap_callback.wrapped_mmap = mtt_allocator_mmap_init_once;
    mtt_internals_create(&mtt_allocator->internals, timestamp, limits,
                         &mmap_callback);

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
    MmapCallback mmap_callback;
    mmap_callback.arg = mtt_allocator;
    mmap_callback.wrapped_mmap = mtt_allocator_mmap_cb_wrapper;
    void *ret =
        mtt_internals_malloc(&mtt_allocator->internals, size, &mmap_callback);
    return ret;
}

MEMKIND_EXPORT void *mtt_allocator_calloc(MTTAllocator *mtt_allocator,
                                          size_t num, size_t size)
{
    size_t alloc_size = num * size;
    void *ret = mtt_allocator_malloc(mtt_allocator, alloc_size);
    if (ret != NULL) {
        memset(ret, 0, alloc_size);
    }
    return ret;
}

MEMKIND_EXPORT void *mtt_allocator_realloc(MTTAllocator *mtt_allocator,
                                           void *ptr, size_t size)
{
    MmapCallback mmap_callback;
    mmap_callback.arg = mtt_allocator;
    mmap_callback.wrapped_mmap = mtt_allocator_mmap_cb_wrapper;
    void *ret = mtt_internals_realloc(&mtt_allocator->internals, ptr, size,
                                      &mmap_callback);
    return ret;
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
    // wait until "running" is entered
    while (mtt_allocator->bgThread.bg_thread_state != THREAD_RUNNING) {
        pthread_cond_wait(&mtt_allocator->bgThread.bg_thread_cond,
                          &mtt_allocator->bgThread.bg_thread_cond_mutex);
    }
    // add FLUSH event
    mtt_allocator->bgThread.bg_thread_state = THREAD_AWAITING_FLUSH;
    // broadcast bg_thread_state update
    pthread_cond_broadcast(&mtt_allocator->bgThread.bg_thread_cond);
    while (mtt_allocator->bgThread.bg_thread_state == THREAD_AWAITING_FLUSH) {
        pthread_cond_wait(&mtt_allocator->bgThread.bg_thread_cond,
                          &mtt_allocator->bgThread.bg_thread_cond_mutex);
    }
    pthread_mutex_unlock(&mtt_allocator->bgThread.bg_thread_cond_mutex);
}

MEMKIND_EXPORT void mtt_allocator_await_balance(MTTAllocator *mtt_allocator)
{
    pthread_mutex_lock(&mtt_allocator->bgThread.bg_thread_cond_mutex);

    // interrupt juggling
    atomic_store(&mtt_allocator->bgThread.interrupt, true);

    // wait until "running" is entered
    while (mtt_allocator->bgThread.bg_thread_state != THREAD_RUNNING) {
        pthread_cond_wait(&mtt_allocator->bgThread.bg_thread_cond,
                          &mtt_allocator->bgThread.bg_thread_cond_mutex);
    }
    // add BALANCE event
    mtt_allocator->bgThread.bg_thread_state = THREAD_AWAITING_BALANCE;
    // broadcast bg_thread_state update
    pthread_cond_broadcast(&mtt_allocator->bgThread.bg_thread_cond);
    while (mtt_allocator->bgThread.bg_thread_state == THREAD_AWAITING_BALANCE) {
        pthread_cond_wait(&mtt_allocator->bgThread.bg_thread_cond,
                          &mtt_allocator->bgThread.bg_thread_cond_mutex);
    }
    pthread_mutex_unlock(&mtt_allocator->bgThread.bg_thread_cond_mutex);
}

MEMKIND_EXPORT void *mtt_allocator_mmap(MTTAllocator *mtt_allocator, void *addr,
                                        size_t length, int prot, int flags,
                                        int fd, off_t offset)
{
    assert(length % TRACED_PAGESIZE == 0ul &&
           "Length is not a multiply of traced pagesize");
    //     assert(prot == PROT_NONE);
    assert(fd == -1 && "Handling of file-backed pages is not supported");
    assert((flags == (MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED)) &&
           "Handling of file-backed/shared memory/ is not supported, "
           "address has to be fixed");
    assert(((uintptr_t)addr) % TRACED_PAGESIZE == 0ul && "Incorrect alignment");
    assert(offset == 0 && "Offset not supported");

    // TODO assert that the same area is not mmapped twice !!!
    // TODO assert that the allocator is initialized
    assert(mtt_allocator->internals.limits.hardLimit &&
           "hard limit cannot be 0!");
    assert(mtt_allocator->internals.limits.softLimit &&
           "soft limit cannot be 0!");
    assert(mtt_allocator->internals.limits.lowLimit <=
               mtt_allocator->internals.limits.softLimit &&
           "soft limit has to be higher than or equal low limit!");
    assert(mtt_allocator->internals.limits.softLimit <=
               mtt_allocator->internals.limits.hardLimit &&
           "hard limit has to be higher than or equal soft limit!");
    // TODO assert that the allocator is not called recursively
    size_t temp_used_dram = 0ul, ndram_size = 0ul;

    // pmem_thresh must be lower than hardLimit for the code to work correctly
    const size_t pmem_thresh = mtt_allocator->internals.limits.hardLimit - 1;

    // add an assert for any person who might possibly change pmem_thresh
    // without reading the comments
    assert(pmem_thresh < mtt_allocator->internals.limits.hardLimit &&
           "pmem_thresh was modified and set to value >= hardLimit; "
           "the code will not work correctly "
           "for hardLimit <= __len < pmem_thresh!");
    if (length >= pmem_thresh) {
        // TODO make an mmap directly on PMEM
        assert(false &&
               "Single allocation greater than or equal to hard limit "
               "is not implemented");
        return NULL;
    }
    do {
        temp_used_dram = atomic_load(&mtt_allocator->internals.usedDram);
        ndram_size = temp_used_dram + length;
        if (temp_used_dram > mtt_allocator->internals.limits.hardLimit) {
            // await until previous operations that surpass the hard_limit are
            // balanced before queuing this one, to prevent hard limit overflow
            // in a manageable case
            mtt_allocator_await_balance(mtt_allocator);
        }
    } while (!atomic_compare_exchange_weak(
        &mtt_allocator->internals.usedDram, &temp_used_dram,
        mtt_allocator->internals.usedDram + length));

    if (ndram_size > mtt_allocator->internals.limits.hardLimit) {
        mtt_allocator_await_balance(mtt_allocator);
    }

    void *ret = mmap(addr, length, prot, flags, fd, offset);
    if (ret) {
        mtt_internals_tracing_multithreaded_push(&mtt_allocator->internals,
                                                 (uintptr_t)addr,
                                                 length / TRACED_PAGESIZE);
    }

    return ret;
}

MEMKIND_EXPORT int mtt_allocator_unmmap(MTTAllocator *mtt_allocator, void *addr,
                                        size_t length)
{
    // TODO handle - wrap, unregister and unmap
    // TODO cannot be async - the pages have to be unregistered before returning
    // from this function!!!
    assert(false && "Not implemented!");
    return -1;
}
