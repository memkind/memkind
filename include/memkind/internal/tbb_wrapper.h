// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2021 Intel Corporation. */

#pragma once
#include <memkind.h>
#include <memkind_deprecated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* dynamically load TBB symbols */
void load_tbb_symbols(void);

/* ops callbacks are replaced by TBB callbacks. */
void tbb_initialize(struct memkind *kind);

/* ptr pointer must come from the valid TBB pool allocation */
void tbb_pool_free_with_kind_detect(void *ptr);

/* ptr pointer must come from the valid TBB pool allocation */
void *tbb_pool_realloc_with_kind_detect(void *ptr, size_t size);

/* ptr pointer must come from the valid TBB pool allocation */
size_t tbb_pool_malloc_usable_size_with_kind_detect(void *ptr);

/* ptr pointer must come from the valid TBB pool allocation */
struct memkind *tbb_detect_kind(void *ptr);

/* update cached stats for TBB (unsupported) */
int tbb_update_cached_stats(void);

/* get allocator stat for TBB (unsupported) */
int tbb_get_global_stat(memkind_stat_type stat, size_t *value);

/* defrag reallocate for TBB (unsupported) */
void *tbb_pool_defrag_reallocate_with_kind_detect(void *ptr);

/* set background threads for TBB (unsupported) */
int tbb_set_bg_threads(bool enable);

#ifdef __cplusplus
}
#endif
