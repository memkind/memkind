// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/ranking.h"
#include "memkind/internal/memkind_private.h"
#include "memkind/internal/ranking_internals.hpp"

// TODO make sure it corresponds to some sensible value, e.g. 5 mins (or 5
// hours)
// unit:nanoseconds
// 3 mins = 3*60 seconds = 3*60*1000 ms = 3*60*1000*1000 us =
// 3*60*1000*1000*1000 ns
#define TIMESTAMP_REFRESH_TIME (3ul * 60ul * 1000ul * 1000ul * 1000ul)

// C bindings ----------------------------------------------------------------

MEMKIND_EXPORT void ranking_create(ranking_handle *handle)
{
    // TODO don't use the default allocator
    *handle = new Ranking();
}

MEMKIND_EXPORT void ranking_destroy(ranking_handle handle)
{
    delete (Ranking *)handle;
}

MEMKIND_EXPORT void ranking_metadata_create(metadata_handle *handle)
{
    // TODO don't use the default allocator
    *handle = new PageMetadata(0, 0.0, 0);
}

MEMKIND_EXPORT void ranking_metadata_destroy(metadata_handle handle)
{
    delete (PageMetadata *)handle;
}

MEMKIND_EXPORT bool ranking_touch(ranking_handle handle, uintptr_t address)
{
    return static_cast<Ranking *>(handle)->Touch(address);
}

MEMKIND_EXPORT void ranking_update(ranking_handle handle, uint64_t timestamp)
{
    uint64_t oldest_timestamp = timestamp > TIMESTAMP_REFRESH_TIME
        ? timestamp - TIMESTAMP_REFRESH_TIME
        : 0ul;
    static_cast<Ranking *>(handle)->Update(timestamp, oldest_timestamp);
}

MEMKIND_EXPORT void ranking_add_pages(ranking_handle handle,
                                      uintptr_t start_address, size_t nof_pages,
                                      uint64_t timestamp)
{
    static_cast<Ranking *>(handle)->AddPages(start_address, nof_pages,
                                             timestamp);
}

MEMKIND_EXPORT size_t ranking_try_remove_pages(ranking_handle handle,
                                               uintptr_t start_address,
                                               size_t nof_pages)
{
    return static_cast<Ranking *>(handle)->TryRemovePages(start_address,
                                                          nof_pages);
}

MEMKIND_EXPORT bool ranking_get_hottest(ranking_handle handle, double *hotness)
{
    return static_cast<Ranking *>(handle)->GetHottest(*hotness);
}

MEMKIND_EXPORT bool ranking_get_coldest(ranking_handle handle, double *hotness)
{
    return static_cast<Ranking *>(handle)->GetColdest(*hotness);
}

MEMKIND_EXPORT void ranking_pop_coldest(ranking_handle handle,
                                        metadata_handle page)
{
    *static_cast<PageMetadata *>(page) =
        static_cast<Ranking *>(handle)->PopColdest();
}

MEMKIND_EXPORT void ranking_pop_hottest(ranking_handle handle,
                                        metadata_handle page)
{
    *static_cast<PageMetadata *>(page) =
        static_cast<Ranking *>(handle)->PopHottest();
}

MEMKIND_EXPORT void ranking_add_page(ranking_handle handle,
                                     metadata_handle page)
{
    static_cast<Ranking *>(handle)->AddPage(*static_cast<PageMetadata *>(page));
}

MEMKIND_EXPORT uintptr_t ranking_get_page_address(metadata_handle page)
{
    return static_cast<PageMetadata *>(page)->GetStartAddr();
}

MEMKIND_EXPORT size_t ranking_get_total_size(ranking_handle handle)
{
    return static_cast<Ranking *>(handle)->GetTotalSize();
}
