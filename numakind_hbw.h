/*
 * Copyright (C) 2014 Intel Corperation.
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

#ifndef numakind_hbw_include_h
#define numakind_hbw_include_h
#ifdef __cplusplus
extern "C" {
#endif

#include "numakind.h"
#include "numakind_default.h"
#include "numakind_arena.h"

static const char * const NUMAKIND_BANDWIDTH_PATH = "/etc/numakind/node-bandwidth";

void numakind_hbw_init(void);
int numakind_hbw_is_available(struct numakind *kind);
int numakind_hbw_hugetlb_get_mmap_flags(struct numakind *kind, int *flags);
int numakind_hbw_preferred_get_mbind_mode(struct numakind *kind, int *mode);
int numakind_hbw_get_mbind_nodemask(struct numakind *kind, unsigned long *nodemask, unsigned long maxnode);
void numakind_hugetlb_init_once(void);
void numakind_hbw_init_once(void);
void numakind_hbw_hugetlb_init_once(void);
void numakind_hbw_preferred_init_once(void);
void numakind_hbw_preferred_hugetlb_init_once(void);

static const struct numakind_ops NUMAKIND_HUGETLB_OPS = {
    .create = numakind_arena_create,
    .destroy = numakind_arena_destroy,
    .malloc = numakind_arena_malloc,
    .calloc = numakind_arena_calloc,
    .posix_memalign = numakind_arena_posix_memalign,
    .realloc = numakind_arena_realloc,
    .free = numakind_default_free,
    .is_available = numakind_default_is_available,
    .mbind = numakind_noop_mbind,
    .get_mmap_flags = numakind_hbw_hugetlb_get_mmap_flags,
    .get_arena = numakind_cpu_get_arena,
    .get_size = numakind_default_get_size,
    .init_once = numakind_hugetlb_init_once
};

static const struct numakind_ops NUMAKIND_HBW_OPS = {
    .create = numakind_arena_create,
    .destroy = numakind_arena_destroy,
    .malloc = numakind_arena_malloc,
    .calloc = numakind_arena_calloc,
    .posix_memalign = numakind_arena_posix_memalign,
    .realloc = numakind_arena_realloc,
    .free = numakind_default_free,
    .is_available = numakind_hbw_is_available,
    .mbind = numakind_default_mbind,
    .get_mmap_flags = numakind_default_get_mmap_flags,
    .get_mbind_mode = numakind_default_get_mbind_mode,
    .get_mbind_nodemask = numakind_hbw_get_mbind_nodemask,
    .get_arena = numakind_cpu_get_arena,
    .get_size = numakind_default_get_size,
    .init_once = numakind_hbw_init_once
};

static const struct numakind_ops NUMAKIND_HBW_HUGETLB_OPS = {
    .create = numakind_arena_create,
    .destroy = numakind_arena_destroy,
    .malloc = numakind_arena_malloc,
    .calloc = numakind_arena_calloc,
    .posix_memalign = numakind_arena_posix_memalign,
    .realloc = numakind_arena_realloc,
    .free = numakind_default_free,
    .is_available = numakind_hbw_is_available,
    .mbind = numakind_default_mbind,
    .get_mmap_flags = numakind_hbw_hugetlb_get_mmap_flags,
    .get_mbind_mode = numakind_default_get_mbind_mode,
    .get_mbind_nodemask = numakind_hbw_get_mbind_nodemask,
    .get_arena = numakind_cpu_get_arena,
    .get_size = numakind_default_get_size,
    .init_once = numakind_hbw_hugetlb_init_once
};

static const struct numakind_ops NUMAKIND_HBW_PREFERRED_OPS = {
    .create = numakind_arena_create,
    .destroy = numakind_arena_destroy,
    .malloc = numakind_arena_malloc,
    .calloc = numakind_arena_calloc,
    .posix_memalign = numakind_arena_posix_memalign,
    .realloc = numakind_arena_realloc,
    .free = numakind_default_free,
    .is_available = numakind_hbw_is_available,
    .mbind = numakind_default_mbind,
    .get_mmap_flags = numakind_default_get_mmap_flags,
    .get_mbind_mode = numakind_hbw_preferred_get_mbind_mode,
    .get_mbind_nodemask = numakind_hbw_get_mbind_nodemask,
    .get_arena = numakind_cpu_get_arena,
    .get_size = numakind_default_get_size,
    .init_once = numakind_hbw_preferred_init_once
};

static const struct numakind_ops NUMAKIND_HBW_PREFERRED_HUGETLB_OPS = {
    .create = numakind_arena_create,
    .destroy = numakind_arena_destroy,
    .malloc = numakind_arena_malloc,
    .calloc = numakind_arena_calloc,
    .posix_memalign = numakind_arena_posix_memalign,
    .realloc = numakind_arena_realloc,
    .free = numakind_default_free,
    .is_available = numakind_hbw_is_available,
    .mbind = numakind_default_mbind,
    .get_mmap_flags = numakind_hbw_hugetlb_get_mmap_flags,
    .get_mbind_mode = numakind_hbw_preferred_get_mbind_mode,
    .get_mbind_nodemask = numakind_hbw_get_mbind_nodemask,
    .get_arena = numakind_cpu_get_arena,
    .get_size = numakind_default_get_size,
    .init_once = numakind_hbw_preferred_hugetlb_init_once
};

#ifdef __cplusplus
}
#endif
#endif
