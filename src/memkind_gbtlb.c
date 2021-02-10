// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_gbtlb.h>
#include <memkind/internal/memkind_hbw.h>
#include <memkind/internal/memkind_hugetlb.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>

static void memkind_hbw_gbtlb_init_once(void);
static void memkind_hbw_preferred_gbtlb_init_once(void);
static void memkind_gbtlb_init_once(void);

static void *gbtlb_mmap(struct memkind *kind, void *addr, size_t size);

MEMKIND_EXPORT struct memkind_ops MEMKIND_HBW_GBTLB_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .mmap = gbtlb_mmap,
    .check_available = memkind_hugetlb_check_available_2mb,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_gbtlb_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HBW_PREFERRED_GBTLB_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .mmap = gbtlb_mmap,
    .check_available = memkind_hugetlb_check_available_2mb,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_mbind_mode = memkind_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_preferred_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_preferred_gbtlb_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_GBTLB_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .mmap = gbtlb_mmap,
    .check_available = memkind_hugetlb_check_available_2mb,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_gbtlb_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

#define ONE_GB 1073741824ULL

static void memkind_gbtlb_ceil_size(size_t *size)
{
    *size = (*size % ONE_GB) ? ((*size / ONE_GB) + 1) * ONE_GB : *size;
}

static void *gbtlb_mmap(struct memkind *kind, void *addr, size_t size)
{
    memkind_gbtlb_ceil_size(&size);
    return memkind_default_mmap(kind, addr, size);
}

static void memkind_hbw_gbtlb_init_once(void)
{
    memkind_init(MEMKIND_HBW_GBTLB, false);
}

static void memkind_hbw_preferred_gbtlb_init_once(void)
{
    memkind_init(MEMKIND_HBW_PREFERRED_GBTLB, false);
}

static void memkind_gbtlb_init_once(void)
{
    memkind_init(MEMKIND_GBTLB, false);
}
