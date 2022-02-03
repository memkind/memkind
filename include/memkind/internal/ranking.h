// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

typedef void *ranking_handle;
typedef struct PageMetadata PageMetadata;

extern void ranking_create(ranking_handle *handle);
extern void ranking_destroy(ranking_handle handle);

extern void ranking_touch(ranking_handle handle, uintptr_t address);
extern void ranking_update(ranking_handle handle, uint64_t timestamp);
extern void ranking_add_pages(ranking_handle handle, uintptr_t start_address,
                              size_t nof_pages, uint64_t timestamp);
/// @param[out] hotness highest hotness value in Ranking
/// @return
///     bool: true if not empty (hotness valid)
extern bool ranking_get_hottest(ranking_handle handle, double *hotness);
/// @param[out] hotness lowest hotness value in Ranking
/// @return
///     bool: true if not empty (hotness valid)
extern bool ranking_get_coldest(ranking_handle handle, double *hotness);
extern void ranking_pop_coldest(ranking_handle handle, PageMetadata *page);
extern void ranking_pop_hottest(ranking_handle handle, PageMetadata *page);
extern void ranking_add_page(ranking_handle handle, PageMetadata *page);

#ifdef __cplusplus
}
#endif
