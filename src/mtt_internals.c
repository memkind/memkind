// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/mtt_internals.h"
#include "memkind/internal/memkind_private.h"

MEMKIND_EXPORT int mtt_internals_create(MttInternals *internals,
                                        uint64_t timestamp)
{
    internals->lastTimestamp = timestamp;
    ranking_create(&internals->dramRanking);
    ranking_create(&internals->pmemRanking);
    return pool_allocator_create(&internals->pool);
}

MEMKIND_EXPORT void mtt_internals_destroy(MttInternals *internals)
{
    ranking_destroy(internals->pmemRanking);
    ranking_destroy(internals->dramRanking);
    pool_allocator_destroy(&internals->pool);
}

MEMKIND_EXPORT void *mtt_internals_malloc(MttInternals *internals, size_t size)
{
    uintptr_t addr = 0;
    size_t nof_pages = 0;
    void *ret =
        pool_allocator_malloc_pages(&internals->pool, size, &addr, &nof_pages);
    if (addr) {
        ranking_add_pages(internals->dramRanking, addr, nof_pages,
                          internals->lastTimestamp);
    }
    return ret;
}

MEMKIND_EXPORT void *mtt_internals_realloc(MttInternals *internals, void *ptr,
                                           size_t size)
{
    // TODO this is more complicated: realloc might call malloc, so we actually
    // need to extend pool
    return NULL;
}

MEMKIND_EXPORT void mtt_internals_free(void *ptr)
{
    // we don't unmap pages, the data is not handled
    pool_allocator_free(ptr);
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
    // update both rankings
    ranking_update(internals->dramRanking, timestamp);
    ranking_update(internals->pmemRanking, timestamp);
    // TODO compare hottest on pmem and coldest on dram!
}
