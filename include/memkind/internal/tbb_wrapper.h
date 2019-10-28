/*
 * Copyright (C) 2017 - 2019 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

#ifdef __cplusplus
}
#endif
