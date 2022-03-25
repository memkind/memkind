// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/mtt_internals.h"
#include "memkind/internal/memkind_log.h"
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

/// @brief Move data bettween hot and cold tiers without changing proportions
///
/// Move data between tiers - demote cold pages from DRAM and promote hot pages
/// from PMEM; the same number of promotions is performed as demotions, hence
/// DRAM/PMEM *proportions* do not change
///
/// @note This function might temporarily increase PMEM usage, but should not
/// increase DRAM usage and should not cause DRAM limit overflow
static void mtt_internals_tiers_juggle(MttInternals *internals)
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

    while (success_dram && success_pmem && (hottest_pmem > coldest_dram) &&
           ranking_get_total_size(internals->dramRanking) > 0ul &&
           ranking_get_total_size(internals->pmemRanking) > 0ul) {
        demote_coldest_dram(internals);
        promote_hottest_pmem(internals);
        success_dram =
            ranking_get_coldest(internals->dramRanking, &coldest_dram);
        success_pmem =
            ranking_get_hottest(internals->pmemRanking, &hottest_pmem);
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

    uintptr_t addr = 0ul;
    size_t nof_pages = 0ul;

    int ret = fast_pool_allocator_create(&internals->pool, &addr, &nof_pages,
                                         user_mmap);
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

MEMKIND_EXPORT void *mtt_internals_malloc(MttInternals *internals, size_t size,
                                          MmapCallback *user_mmap)
{
    uintptr_t addr[2] = {0ul};
    size_t nof_pages[2] = {0ul};
    void *ret = fast_pool_allocator_malloc_pages(
        &internals->pool, size, (uintptr_t *)addr, (size_t *)nof_pages,
        user_mmap);
    for (size_t i = 0ul; i < 2u; ++i) {
        if (addr[i]) {
            mmap_tracing_queue_multithreaded_push(&internals->mmapTracingQueue,
                                                  addr[i], nof_pages[i]);
        }
    }
    return ret;
}

MEMKIND_EXPORT void *mtt_internals_realloc(MttInternals *internals, void *ptr,
                                           size_t size, MmapCallback *user_mmap)
{
    if (size == 0) {
        mtt_internals_free(internals, ptr);
        return NULL;
    } else if (ptr == NULL) {
        return mtt_internals_malloc(internals, size, user_mmap);
    }

    size_t old_size = mtt_internals_usable_size(internals, ptr);
    if (size == old_size) {
        return ptr;
    }

    void *ret = mtt_internals_malloc(internals, size, user_mmap);
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

MEMKIND_EXPORT size_t mtt_internals_ranking_balance(MttInternals *internals,
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

MEMKIND_EXPORT void mtt_internals_ranking_update(MttInternals *internals,
                                                 uint64_t timestamp,
                                                 atomic_size_t *used_dram)
{
    // 1. Add new mappings to rankings
    MMapTracingNode *node =
        mmap_tracing_queue_multithreaded_take_all(&internals->mmapTracingQueue);
    uintptr_t start_addr;
    size_t nof_pages;
    while (mmap_tracing_queue_process_one(&node, &start_addr, &nof_pages)) {
        ranking_add_pages(internals->dramRanking, start_addr, nof_pages,
                          internals->lastTimestamp);
    }
    // 2. Update both rankings - hotness
    ranking_update(internals->dramRanking, timestamp);
    ranking_update(internals->pmemRanking, timestamp);
    // 3. Handle soft and hard limit - move pages dram vs pmem
    size_t surplus = mtt_internals_ranking_balance(internals, used_dram,
                                                   internals->limits.softLimit);
    if (surplus)
        log_info("Not able to balance DRAM usage down to softLimit at once,"
                 " %lu left unmoved;"
                 " was a single allocation > lowLimit requested?",
                 surplus);
    // 4. Handle hottest on PMEM vs coldest on dram page movement
    mtt_internals_tiers_juggle(internals);
}
