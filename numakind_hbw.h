/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
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
    .get_arena = numakind_cpu_get_arena
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
    .get_arena = numakind_cpu_get_arena
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
    .get_arena = numakind_cpu_get_arena
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
    .get_arena = numakind_cpu_get_arena
};

#ifdef __cplusplus
}
#endif
#endif
