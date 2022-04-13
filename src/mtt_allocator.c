// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <memkind/internal/memkind_dax_kmem.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/pebs.h>
#include <memkind/internal/ranking_utils.h>
#include <memkind/internal/timespec_ops.h>

#include <mtt_allocator.h>

#include <assert.h>
#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>

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
static pthread_once_t mtt_register_atfork_once = PTHREAD_ONCE_INIT;

extern nodemask_t nodemasks[2];
static pthread_once_t nodemask_inits[2] = {PTHREAD_ONCE_INIT,
                                           PTHREAD_ONCE_INIT};

// static function declarations -----------------------------------------------

static void init_dram_nodemask_once();
static void init_dax_kmem_nodemask_once();

void (*init_routines[2])(void) = {init_dram_nodemask_once,
                                  init_dax_kmem_nodemask_once};

static void *mtt_run_bg_thread(void *mtt_alloc);
/// @pre allocatorsMutex must be acquired before calling
static void touch_callback(uintptr_t address, uint64_t timestamp);
/// @pre allocatorsMutex must be acquired before calling
static void update_rankings();
static void balance_rankings();
static void register_mtt_allocator(MTTAllocator *mtt_allocator);
static void unregister_mtt_allocator(MTTAllocator *mtt_allocator);
static void *mtt_allocator_mmap(void *arg, void *__addr, size_t __len,
                                int __prot, int __flags, int __fd,
                                __off_t __offset);
static void background_thread_start(BackgroundThread *bg_thread);
static void background_thread_stop(MTTAllocator *mtt_allocator);

// static function definitions ------------------------------------------------

static void init_dram_nodemask_once()
{
    unsigned i;

    int err = numa_available();
    if (err) {
        log_fatal("NUMA is not supported (error code:%d)", err);
        abort();
    }

    unsigned nodes_num = (unsigned)numa_max_node() + 1;
    struct bitmask *node_cpus = numa_allocate_cpumask();
    struct bitmask *dram_nodes_bm = numa_allocate_nodemask();

    for (i = 0; i < nodes_num; i++) {
        int ret = numa_node_to_cpus(i, node_cpus);
        if (ret == 0 && numa_bitmask_weight(node_cpus))
            numa_bitmask_setbit(dram_nodes_bm, i);
    }
    numa_bitmask_free(node_cpus);

    copy_bitmask_to_nodemask(dram_nodes_bm, &nodemasks[DRAM]);
}

static void init_dax_kmem_nodemask_once()
{
    struct bitmask *bitmask = NULL;
    int bm_weight = 0;

    int err = numa_available();
    if (err) {
        log_fatal("NUMA is not supported (error code:%d)", err);
        abort();
    }
    bitmask = numa_allocate_nodemask();

    int status = memkind_dax_kmem_all_get_mbind_nodemask(
        NULL, nodemasks[DAX_KMEM].n, NUMA_NUM_NODES);
    if (status == MEMKIND_ERROR_OPERATION_FAILED) {
        log_fatal(
            "memkind wasn't build with a minimum required libdaxctl-devel version");
        abort();
    } else if (status != MEMKIND_SUCCESS) {
        log_fatal("Failed to get DAX KMEM nodes nodemask");
        abort();
    }

    copy_nodemask_to_bitmask(&nodemasks[DAX_KMEM], bitmask);
    bm_weight = numa_bitmask_weight(bitmask);
    if (bm_weight == 0) {
        log_fatal(
            "Page movement to DAX_KMEM failed: no DAX_KMEM nodes detected");
        abort();
    }
}

/// @brief Wrapper for mmap with PMEM/DRAM balancing
///
/// @note At the function exit, we should have at most hardLimit data in DRAM
static void *mtt_allocator_mmap(void *arg, void *__addr, size_t __len,
                                int __prot, int __flags, int __fd,
                                __off_t __offset)
{
    MTTAllocator *allocator = arg;
    // TODO assert that the same area is not mmapped twice !!!
    // TODO assert that the allocator is initialized
    assert(allocator->internals.limits.hardLimit && "hard limit cannot be 0!");
    assert(allocator->internals.limits.softLimit && "soft limit cannot be 0!");
    assert(allocator->internals.limits.lowLimit <=
               allocator->internals.limits.softLimit &&
           "soft limit has to be higher than or equal low limit!");
    assert(allocator->internals.limits.softLimit <=
               allocator->internals.limits.hardLimit &&
           "hard limit has to be higher than or equal soft limit!");
    // TODO assert that the allocator is not called recursively
    size_t temp_used_dram = 0ul, ndram_size = 0ul;

    // pmem_thresh must be lower than hardLimit for the code to work correctly
    const size_t pmem_thresh = allocator->internals.limits.hardLimit - 1;

    // add an assert for any person who might possibly change pmem_thresh
    // without reading the comments
    assert(pmem_thresh < allocator->internals.limits.hardLimit &&
           "pmem_thresh was modified and set to value >= hardLimit; "
           "the code will not work correctly "
           "for hardLimit <= __len < pmem_thresh!");
    if (__len >= pmem_thresh) {
        // TODO make an mmap directly on PMEM
        assert(false && "not implemented");
        return NULL;
    }
    do {
        temp_used_dram = atomic_load(&allocator->internals.usedDram);
        ndram_size = temp_used_dram + __len;
        if (temp_used_dram > allocator->internals.limits.hardLimit) {
            // await until previous operations that surpass the hard_limit are
            // balanced before queuing this one, to prevent hard limit overflow
            // in a manageable case
            mtt_allocator_await_balance(allocator);
        }
    } while (!atomic_compare_exchange_weak(
        &allocator->internals.usedDram, &temp_used_dram,
        allocator->internals.usedDram + __len));

    if (ndram_size > allocator->internals.limits.hardLimit) {
        mtt_allocator_await_balance(allocator);
    }

    return mmap(__addr, __len, __prot, __flags, __fd, __offset);
}

/// @brief Wrapper for mmap
///
/// @pre @p arg (MTTAllocator*) should have the following fields initialized:
///     - allocator->internals.usedDram,
///     - allocator->internals.limits.hardLimit
static void *mtt_allocator_mmap_init_once(void *arg, void *__addr, size_t __len,
                                          int __prot, int __flags, int __fd,
                                          __off_t __offset)
{
    MTTAllocator *allocator = arg;
    // TODO assert that the same area is not mmapped twice !!!
    // TODO assert that the allocator is initialized
    // TODO assert that the allocator is not called recursively
    size_t prev_used_dram =
        atomic_fetch_add(&allocator->internals.usedDram, __len);

    assert(prev_used_dram == 0ul &&
           "This wrapper is only supposed to be used once, at init time!");
    assert(__len < allocator->internals.limits.hardLimit &&
           "Balancing not available at init time, increase hard limit!");
    return mmap(__addr, __len, __prot, __flags, __fd, __offset);
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
    timespec_get_time(CLOCK_MONOTONIC, &end_time);

    pthread_mutex_lock(bg_thread_cond_mutex);
    bg_thread->bg_thread_state = THREAD_RUNNING;
    pthread_cond_broadcast(&bg_thread->bg_thread_cond);
    pthread_mutex_unlock(bg_thread_cond_mutex);

    ThreadState_t bg_thread_state = THREAD_RUNNING;
    while (bg_thread_state != THREAD_FINISHED) {
        pthread_mutex_lock(bg_thread_cond_mutex);
        bg_thread_state = bg_thread->bg_thread_state;
        // perform sleep only if no sleep interrupt was scheduled
        if (bg_thread_state == THREAD_RUNNING) {
            // wait until next occurrence (according to frequency) or interrupt
            pthread_cond_timedwait(&bg_thread->bg_thread_cond,
                                   &bg_thread->bg_thread_cond_mutex, &end_time);
        }
        pthread_mutex_unlock(bg_thread_cond_mutex);

        // calculate time to wait - only what remains after current iteration
        timespec_get_time(CLOCK_MONOTONIC, &end_time);
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
                update_rankings();
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
    update_rankings(); // flush mmapTracingQueue
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

static void update_rankings()
{
    MTTAllocatorNode *cnode = root;
    while (cnode) {
        mtt_internals_ranking_update(&cnode->allocator->internals,
                                     last_timestamp,
                                     &cnode->allocator->internals.usedDram);
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
    pthread_once(&nodemask_inits[DRAM], init_routines[DRAM]);
    pthread_once(&nodemask_inits[DAX_KMEM], init_routines[DAX_KMEM]);

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
    mmap_callback.wrapped_mmap = mtt_allocator_mmap;
    void *ret =
        mtt_internals_malloc(&mtt_allocator->internals, size, &mmap_callback);
    return ret;
}

MEMKIND_EXPORT void *mtt_allocator_realloc(MTTAllocator *mtt_allocator,
                                           void *ptr, size_t size)
{
    MmapCallback mmap_callback;
    mmap_callback.arg = mtt_allocator;
    mmap_callback.wrapped_mmap = mtt_allocator_mmap;
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
