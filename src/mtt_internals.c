// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/mtt_internals.h"
#include "memkind/internal/bigary.h"
#include "memkind/internal/memkind_log.h"
#include "memkind/internal/memkind_private.h"
#include "memkind/internal/mmap_tracing_queue.h"
#include "memkind/internal/multithreaded_touch_queue.h"
#include "memkind/internal/pagesizes.h"
#include "memkind/internal/ranking.h"
#include "memkind/internal/ranking_utils.h"

#include "assert.h"
#include "stdint.h"
#include "string.h"
#include "threads.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))

#if USE_FAST_POOL
#define POOL_ALLOCATOR_TYPE(x) fast_pool_allocator_##x
#else
#define POOL_ALLOCATOR_TYPE(x) pool_allocator_##x
#endif

// with 4kB traced page, 512kB per cycle
#define MAX_TO_JUGGLE (128ul)
// do not juggle pages with hotness diff below 10%
#define MIN_JUGGLE_DIFF (0.10)

#define MALLOC_TOUCHES_TO_SKIP (10ul)

// static functions -----------------------------------------------------------

static void malloc_touch(MttInternals *internals, uintptr_t address)
{
    static thread_local size_t g_mallocTouchCounter = 0ul;
    if (++g_mallocTouchCounter > MALLOC_TOUCHES_TO_SKIP) {
        multithreaded_touch_queue_multithreaded_push(&internals->touchQueue,
                                                     address);
        g_mallocTouchCounter = 0ul;
    }
}

static void promote_hottest_pmem(MttInternals *internals)
{
    // promote (PMEM->DRAM)
    ranking_pop_hottest(internals->pmemRanking, internals->tempMetadataHandle);
    uintptr_t start_addr =
        ranking_get_page_address(internals->tempMetadataHandle);
    ranking_add_page(internals->dramRanking, internals->tempMetadataHandle);
    int ret = move_page_metadata(start_addr, DRAM);
    assert(ret == MEMKIND_SUCCESS && "mbind data movement failed");
}

static void demote_coldest_dram(MttInternals *internals)
{
    // demote (DRAM->PMEM)
    ranking_pop_coldest(internals->dramRanking, internals->tempMetadataHandle);
    uintptr_t start_addr =
        ranking_get_page_address(internals->tempMetadataHandle);
    ranking_add_page(internals->pmemRanking, internals->tempMetadataHandle);
    int ret = move_page_metadata(start_addr, DAX_KMEM);
    assert(ret == MEMKIND_SUCCESS && "mbind data movement failed");
}

/// @brief Move data bettween hot and cold tiers without changing proportions
///
/// Move data between tiers - demote cold pages from DRAM and promote hot pages
/// from PMEM; the same number of promotions is performed as demotions, hence
/// DRAM/PMEM *proportions* do not change
///
/// @note This function might temporarily increase PMEM usage, but should not
/// increase DRAM usage and should not cause DRAM limit overflow
/*static */ size_t mtt_internals_tiers_juggle(MttInternals *internals,
                                              atomic_bool *interrupt)
{
    // Handle hottest on PMEM vs coldest on dram page movement
    // different approaches possible, e.g.
    //      a) move when avg (hottest dram, coldest dram) < hottest dram
    //      b) move while coldest dram is colder than hottest pmem
    // Approach b) was chosen, at least for now
    double coldest_dram = -1.0;
    double hottest_pmem = -1.0;
    bool success_dram =
        ranking_get_coldest(internals->dramRanking, &coldest_dram);
    bool success_pmem =
        ranking_get_hottest(internals->pmemRanking, &hottest_pmem);

    size_t juggle_it = 0ul;
    while (juggle_it++ < MAX_TO_JUGGLE && success_dram && success_pmem &&
           (hottest_pmem * (1.0 - MIN_JUGGLE_DIFF) > coldest_dram) &&
           ranking_get_total_size(internals->dramRanking) > 0ul &&
           ranking_get_total_size(internals->pmemRanking) > 0ul) {
        demote_coldest_dram(internals);
        promote_hottest_pmem(internals);
        success_dram =
            ranking_get_coldest(internals->dramRanking, &coldest_dram);
        success_pmem =
            ranking_get_hottest(internals->pmemRanking, &hottest_pmem);

        if (atomic_load(interrupt))
            break;
    }

    return --juggle_it;
}

static size_t mtt_internals_ranking_balance_internal(MttInternals *internals,
                                                     atomic_size_t *used_dram,
                                                     size_t dram_limit)
{
    size_t ret = 0ul;
    // Handle soft and hard limit - move pages dram vs pmem
    size_t temp_dram_size = atomic_load(used_dram);
    if (dram_limit < temp_dram_size) {
        // move (demote) DRAM to PMEM
        size_t size_to_move = temp_dram_size - dram_limit;
        assert(size_to_move % TRACED_PAGESIZE == 0 &&
               "ranking size is not a multiply of TRACED_PAGESIZE");
        size_t dram_ranking_size =
            ranking_get_total_size(internals->dramRanking);
        if (size_to_move > dram_ranking_size) {
            ret = size_to_move - dram_ranking_size;
            size_to_move = dram_ranking_size;
        }
        assert(size_to_move <= ranking_get_total_size(internals->dramRanking) &&
               "a demotion bigger than DRAM ranking requested, case not handled"
               " hard/soft_limit too low,"
               " queued mmap(s) should go directly to pmem");
        assert(*used_dram >= ranking_get_total_size(internals->dramRanking) &&
               "internal error; dram size, including queued data,"
               " should not be higher than dram ranking size");
        size_t nof_pages_to_move = size_to_move / TRACED_PAGESIZE;
        for (size_t i = 0; i < nof_pages_to_move; ++i) {
            demote_coldest_dram(internals);
        }
        atomic_fetch_sub(used_dram, size_to_move);
        assert((int64_t)*used_dram >= 0 && "internal error, moved too much");
    } else if (temp_dram_size + TRACED_PAGESIZE < internals->limits.lowLimit) {

        // move (promote) PMEM to DRAM
        // incease dram_size before movement to avoid race conditions &
        // hard_limit overflow

        //         TODO would be perfect to perform the check
        //         assert(size_to_move % TRACED_PAGESIZE == 0 &&
        //                "ranking size is not a multiply of TRACED_PAGESIZE");

        temp_dram_size =
            atomic_fetch_add(used_dram, TRACED_PAGESIZE) + TRACED_PAGESIZE;

        while (ranking_get_total_size(internals->pmemRanking) > 0ul &&
               temp_dram_size < internals->limits.lowLimit) {
            promote_hottest_pmem(internals);
            temp_dram_size =
                atomic_fetch_add(used_dram, TRACED_PAGESIZE) + TRACED_PAGESIZE;
        }
        // last iteration increases dram_size, but movement is not performed -
        // the value should be decreased back
        (void)atomic_fetch_sub(used_dram, TRACED_PAGESIZE);
    }

    return ret;
}

static ssize_t mtt_internals_process_queued_mman(MttInternals *internals)
{
    // process queued mmaps
    MMapTracingNode *node =
        mmap_tracing_queue_multithreaded_take_all(&internals->mmapTracingQueue);
    uintptr_t start_addr;
    size_t nof_pages;
    MMapTracingEvent_e event;
    ssize_t dram_pages_added = 0l;
    while (mmap_tracing_queue_process_one(&node, &start_addr, &nof_pages,
                                          &event)) {
        switch (event) {
            case MMAP_TRACING_EVENT_MMAP:
                ranking_add_pages(internals->dramRanking, start_addr, nof_pages,
                                  internals->lastTimestamp);
                break;
            case MMAP_TRACING_EVENT_RE_MMAP:
                for (uintptr_t tstart_addr = start_addr;
                     tstart_addr < start_addr + nof_pages * TRACED_PAGESIZE;
                     tstart_addr += TRACED_PAGESIZE) {
                    memory_type_t mem_type = get_page_memory_type(tstart_addr);
                    ranking_handle ranking_to_add =
                        NULL; // initialize - silence gcc warning
                    switch (mem_type) {
                        case DRAM:
                            ranking_to_add = internals->dramRanking;
                            ++dram_pages_added;
                            break;
                        case DAX_KMEM:
                            ranking_to_add = internals->pmemRanking;
                            break;
                    }
                    ranking_add_pages(ranking_to_add, tstart_addr, 1ul,
                                      internals->lastTimestamp);
                }
                break;
            case MMAP_TRACING_EVENT_MUNMAP:
            {
                size_t removed_dram = ranking_try_remove_pages(
                    internals->dramRanking, start_addr, nof_pages);
                size_t removed_pmem = ranking_try_remove_pages(
                    internals->pmemRanking, start_addr, nof_pages);
                size_t removed_total = removed_dram + removed_pmem;
                if (removed_total != nof_pages)
                    log_info("only removed %lu/%lu pages on unmap!",
                             removed_total, nof_pages);
                dram_pages_added -= (ssize_t)removed_dram;
            } break;
        }
    }

    return dram_pages_added;
}

static void mtt_internals_process_queued_touch(MttInternals *internals)
{
    MultithreadedTouchNode *mt_node =
        multithreaded_touch_queue_multithreaded_take_all(
            &internals->touchQueue);
    uintptr_t address;
    while (multithreaded_touch_queue_process_one(&mt_node, &address)) {
        mtt_internals_touch(internals, address);
    }
}

static void touch_tracker_zero(TouchTracker *tracker)
{
    tracker->touchesDRAM = 0ul;
    tracker->touchesPMEM = 0ul;
}

static void touch_tracker_update_touches(TouchTracker *tracker, bool dram_touch,
                                         bool pmem_touch)
{
    assert(!(dram_touch && pmem_touch));
    if (dram_touch)
        tracker->touchesDRAM++;
    else if (pmem_touch)
        tracker->touchesPMEM++;

    const size_t INTERVAL = 1000ul;
    if (tracker->touchesDRAM + tracker->touchesPMEM >= INTERVAL) {
        assert(tracker->touchesDRAM + tracker->touchesPMEM == INTERVAL);
        log_info(
            "touches DRAM: %lu.%.3lu, PMEM: %lu.%.3lu",
            tracker->touchesDRAM / INTERVAL, tracker->touchesDRAM % INTERVAL,
            tracker->touchesPMEM / INTERVAL, tracker->touchesPMEM % INTERVAL);
        touch_tracker_zero(tracker);
    }
}

// global functions -----------------------------------------------------------

MEMKIND_EXPORT int mtt_internals_create(MttInternals *internals,
                                        uint64_t timestamp,
                                        const MTTInternalsLimits *limits,
                                        MmapCallback *user_mmap)
{
    // TODO think about default limits value
    internals->usedDram = 0ul;
    internals->lastTimestamp = timestamp;
    assert(limits->lowLimit <= limits->softLimit &&
           "low limit (movement PMEM -> DRAM occurs below) "
           " has to be lower than or equal to "
           " soft limit (movement DRAM -> PMEM occurs above)");
    assert(limits->softLimit <= limits->hardLimit &&
           "soft limit (movement DRAM -> PMEM occurs above) "
           " has to be lower than or equal to "
           " hard limit (any allocation that surpasses this limit "
           " should be placed on PMEM TODO not implemented)");
    assert(limits->softLimit > 0ul &&
           "soft & hard limits have to be positive!");
    assert(limits->hardLimit % TRACED_PAGESIZE == 0 &&
           "hard limit is not a multiple of TRACED_PAGESIZE");
    assert(limits->softLimit % TRACED_PAGESIZE == 0 &&
           "soft limit is not a multiple of TRACED_PAGESIZE");
    assert(limits->lowLimit % TRACED_PAGESIZE == 0 &&
           "low limit is not a multiple of TRACED_PAGESIZE");
    internals->limits = *limits;
    ranking_create(&internals->dramRanking);
    ranking_create(&internals->pmemRanking);
    ranking_metadata_create(&internals->tempMetadataHandle);
    mmap_tracing_queue_create(&internals->mmapTracingQueue);
    multithreaded_touch_queue_create(&internals->touchQueue);
    touch_tracker_zero(&internals->tracker);
    int ret = POOL_ALLOCATOR_TYPE(create)(&internals->pool, user_mmap);

    return ret;
}

MEMKIND_EXPORT void mtt_internals_destroy(MttInternals *internals)
{
    ranking_metadata_destroy(internals->tempMetadataHandle);
    ranking_destroy(internals->pmemRanking);
    ranking_destroy(internals->dramRanking);
    mmap_tracing_queue_destroy(&internals->mmapTracingQueue);
    multithreaded_touch_queue_destroy(&internals->touchQueue);
    // internals are already destroyed, no need to track mmap/munmap
    POOL_ALLOCATOR_TYPE(destroy)(&internals->pool, &gStandardMmapCallback);
}

MEMKIND_EXPORT void *mtt_internals_malloc(MttInternals *internals, size_t size,
                                          MmapCallback *user_mmap)
{
    void *ret =
        POOL_ALLOCATOR_TYPE(malloc_mmap)(&internals->pool, size, user_mmap);
    malloc_touch(internals, (uintptr_t)ret);
    return ret;
}

MEMKIND_EXPORT void *mtt_internals_realloc(MttInternals *internals, void *ptr,
                                           size_t size, MmapCallback *user_mmap)
{
    void *ret = POOL_ALLOCATOR_TYPE(realloc_mmap)(&internals->pool, ptr, size,
                                                  user_mmap);
    malloc_touch(internals, (uintptr_t)ret);
    return ret;
}

MEMKIND_EXPORT void mtt_internals_free(MttInternals *internals, void *ptr)
{
    // TODO add & handle unmap during productization stage!
    // we don't unmap pages, the data is not handled
    POOL_ALLOCATOR_TYPE(free)(&internals->pool, ptr);
}

MEMKIND_EXPORT size_t mtt_internals_usable_size(MttInternals *internals,
                                                void *ptr)
{
    return POOL_ALLOCATOR_TYPE(usable_size)(&internals->pool, ptr);
}

MEMKIND_EXPORT void mtt_internals_touch(MttInternals *internals,
                                        uintptr_t address)
{
    // touch both - if ranking does not contain the page,
    // the touch will be ignored
    //
    // touch on unknown address is immediately dropped in O(1) time
    bool dram_touch = ranking_touch(internals->dramRanking, address);
    bool pmem_touch = ranking_touch(internals->pmemRanking, address);
    touch_tracker_update_touches(&internals->tracker, dram_touch, pmem_touch);
}

MEMKIND_EXPORT size_t mtt_internals_ranking_balance(MttInternals *internals,
                                                    atomic_size_t *used_dram,
                                                    size_t dram_limit)
{
    // update dram ranking
    ssize_t diff_in_dram_used = mtt_internals_process_queued_mman(internals);
    // handle issues with signed -> unsigned casting
    if (diff_in_dram_used >= 0) {
        atomic_fetch_add(used_dram, diff_in_dram_used);
    } else {
        atomic_fetch_sub(used_dram, -diff_in_dram_used);
    }
    return mtt_internals_ranking_balance_internal(internals, used_dram,
                                                  dram_limit);
}

MEMKIND_EXPORT void mtt_internals_ranking_update(MttInternals *internals,
                                                 uint64_t timestamp,
                                                 atomic_size_t *used_dram,
                                                 atomic_bool *interrupt)
{
    internals->lastTimestamp = timestamp;
    // 1. Add new mappings to rankings
    ssize_t additional_dram_used = mtt_internals_process_queued_mman(internals);
    // handle issues with signed -> unsigned casting
    if (additional_dram_used >= 0) {
        atomic_fetch_add(used_dram, additional_dram_used);
    } else {
        atomic_fetch_sub(used_dram, -additional_dram_used);
    }
    // 2. Update both rankings - hotness
    if (atomic_load(interrupt)) {
        return;
    }
    mtt_internals_process_queued_touch(internals);
    if (atomic_load(interrupt)) {
        return;
    }
    ranking_update(internals->dramRanking, timestamp);
    if (atomic_load(interrupt)) {
        return;
    }
    ranking_update(internals->pmemRanking, timestamp);
    // 3. Handle soft and hard limit - move pages dram vs pmem
    size_t surplus = mtt_internals_ranking_balance(internals, used_dram,
                                                   internals->limits.softLimit);
    if (surplus)
        log_info("Not able to balance DRAM usage down to softLimit at once,"
                 " %lu left unmoved;"
                 " was a single allocation > lowLimit requested?",
                 surplus);
    if (atomic_load(interrupt)) {
        return;
    }
    // 4. Handle hottest on PMEM vs coldest on dram page movement
    size_t juggled = mtt_internals_tiers_juggle(internals, interrupt);
    log_info("juggled %lu", juggled);
}

MEMKIND_EXPORT void
mtt_internals_tracing_multithreaded_push(MttInternals *internals,
                                         uintptr_t addr, size_t nof_pages,
                                         MMapTracingEvent_e event)
{
    mmap_tracing_queue_multithreaded_push(&internals->mmapTracingQueue, addr,
                                          nof_pages, event);
}
