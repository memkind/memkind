// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/ranking.h"
#include "memkind/internal/memkind_private.h"
#include "memkind/internal/ranking_internals.hpp"

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

MEMKIND_EXPORT void ranking_touch(ranking_handle handle, uintptr_t address)
{
    static_cast<Ranking *>(handle)->Touch(address);
}

MEMKIND_EXPORT void ranking_update(ranking_handle handle, uint64_t timestamp)
{
    static_cast<Ranking *>(handle)->Update(timestamp);
}

MEMKIND_EXPORT void ranking_add_pages(ranking_handle handle,
                                      uintptr_t start_address, size_t nof_pages,
                                      uint64_t timestamp)
{
    static_cast<Ranking *>(handle)->AddPages(start_address, nof_pages,
                                             timestamp);
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

MEMKIND_EXPORT size_t ranking_get_total_size(ranking_handle handle)
{
    return static_cast<Ranking *>(handle)->GetTotalSize();
}
