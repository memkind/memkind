// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_interleave.h>
#include <memkind/internal/memkind_private.h>

MEMKIND_EXPORT struct memkind_ops MEMKIND_INTERLEAVE_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .mbind = memkind_default_mbind,
    .madvise = memkind_nohugepage_madvise,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_interleave_get_mbind_mode,
    .get_mbind_nodemask = memkind_default_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_interleave_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT void memkind_interleave_init_once(void)
{
    memkind_init(MEMKIND_INTERLEAVE, true);
}
