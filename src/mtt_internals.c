// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/mtt_internals.h"
#include "memkind/internal/memkind_private.h"
#include "memkind/internal/pagesizes.h"
#include "memkind/internal/ranking_utils.h"

#include "assert.h"
#include "string.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))

// static functions -----------------------------------------------------------

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

// global functions -----------------------------------------------------------

MEMKIND_EXPORT int mtt_internals_create(MttInternals *internals,
                                        uint64_t timestamp,
                                        const MTTInternalsLimits *limits)
{
    // TODO think about default limits value
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

    uintptr_t addr = 0ul;
    size_t nof_pages = 0ul;

    int ret = fast_pool_allocator_create(&internals->pool, &addr, &nof_pages);
    if (addr)
        mmap_tracing_queue_multithreaded_push(&internals->mmapTracingQueue,
                                              addr, nof_pages);

    return ret;
}

MEMKIND_EXPORT void mtt_internals_destroy(MttInternals *internals)
{
    ranking_metadata_destroy(internals->tempMetadataHandle);
    ranking_destroy(internals->pmemRanking);
    ranking_destroy(internals->dramRanking);
    mmap_tracing_queue_destroy(&internals->mmapTracingQueue);
    fast_pool_allocator_destroy(&internals->pool);
}

MEMKIND_EXPORT void *mtt_internals_malloc(MttInternals *internals, size_t size)
{
    uintptr_t addr[2] = {0ul};
    size_t nof_pages[2] = {0ul};
    void *ret = fast_pool_allocator_malloc_pages(
        &internals->pool, size, (uintptr_t *)addr, (size_t *)nof_pages);
    for (size_t i = 0ul; i < 2u; ++i) {
        if (addr[i]) {
            mmap_tracing_queue_multithreaded_push(&internals->mmapTracingQueue,
                                                  addr[i], nof_pages[i]);
        }
    }
    return ret;
}

MEMKIND_EXPORT void *mtt_internals_realloc(MttInternals *internals, void *ptr,
                                           size_t size)
{
    // TODO this is more complicated: realloc might call malloc, so we actually
    // need to extend pool

    if (size == 0) {
        mtt_internals_free(internals, ptr);
        return NULL;
    } else if (ptr == NULL) {
        return mtt_internals_malloc(internals, size);
    }

    size_t old_size = mtt_internals_usable_size(internals, ptr);
    if (size == old_size) {
        return ptr;
    }

    void *ret = mtt_internals_malloc(internals, size);
    if (ret) {
        memcpy(ret, ptr, old_size);
        mtt_internals_free(internals, ptr); 
    }

    return ret;
}

MEMKIND_EXPORT void mtt_internals_free(MttInternals *internals, void *ptr)
{
    // TODO add & handle unmap during productization stage!
    // we don't unmap pages, the data is not handled
    fast_pool_allocator_free(&internals->pool, ptr);
}

MEMKIND_EXPORT size_t mtt_internals_usable_size(MttInternals *internals,
                                                void *ptr)
{
    return fast_pool_allocator_usable_size(&internals->pool, ptr);
}

MEMKIND_EXPORT void mtt_internals_touch(MttInternals *internals,
                                        uintptr_t address)
{
    // touch both - if ranking does not contain the page,
    // the touch will be ignored
    //
    // touch on unknown address is immediately dropped in O(1) time
    ranking_touch(internals->dramRanking, address);
    ranking_touch(internals->pmemRanking, address);
}

MEMKIND_EXPORT void mtt_internals_ranking_update(MttInternals *internals,
                                                 uint64_t timestamp)
{
    MMapTracingNode *node =
        mmap_tracing_queue_multithreaded_take_all(&internals->mmapTracingQueue);
    uintptr_t start_addr;
    size_t nof_pages;
    while (mmap_tracing_queue_process_one(&node, &start_addr, &nof_pages)) {
        ranking_add_pages(internals->dramRanking, start_addr, nof_pages,
                          internals->lastTimestamp);
    }

    // update both rankings
    ranking_update(internals->dramRanking, timestamp);
    ranking_update(internals->pmemRanking, timestamp);
    // 1. Handle soft and hard limit - move pages dram vs pmem
    size_t dram_size = ranking_get_total_size(internals->dramRanking);
    if (dram_size < internals->limits.lowLimit) {
        // move (promote) PMEM to DRAM
        size_t pmem_size = ranking_get_total_size(internals->pmemRanking);
        size_t size_gap_dram = internals->limits.lowLimit - dram_size;
        size_t size_to_move = min(size_gap_dram, pmem_size);
        assert(size_to_move % TRACED_PAGESIZE == 0 &&
               "ranking size is not a multiply of TRACED_PAGESIZE");
        size_t nof_pages_to_move = size_to_move / TRACED_PAGESIZE;
        for (size_t i = 0; i < nof_pages_to_move; ++i) {
            promote_hottest_pmem(internals);
        }
    } else if (internals->limits.softLimit < dram_size) {
        // move (demote) DRAM to PMEM
        size_t size_to_move = dram_size - internals->limits.softLimit;
        assert(size_to_move % TRACED_PAGESIZE == 0 &&
               "ranking size is not a multiply of TRACED_PAGESIZE");
        size_t nof_pages_to_move = size_to_move / TRACED_PAGESIZE;
        for (size_t i = 0; i < nof_pages_to_move; ++i) {
            demote_coldest_dram(internals);
        }
    }
    // 2. Handle hottest on PMEM vs coldest on dram page movement
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

    while (success_dram && success_pmem && (hottest_pmem > coldest_dram)) {
        demote_coldest_dram(internals);
        promote_hottest_pmem(internals);
        success_dram =
            ranking_get_coldest(internals->dramRanking, &coldest_dram);
        success_pmem =
            ranking_get_hottest(internals->pmemRanking, &hottest_pmem);
    }
}
