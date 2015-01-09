/*
 * Copyright (C) 2014, 2015 Intel Corporation.
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

#include <assert.h>
#include <sys/mman.h>
#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
#ifndef MAP_HUGE_2MB
#define MAP_HUGE_2MB (21 << 26)
#endif

#include "memkind_hugetlb.h"
#include "memkind_default.h"
#include "memkind_arena.h"

const struct memkind_ops MEMKIND_HUGETLB_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_arena = memkind_thread_get_arena,
    .get_size = memkind_default_get_size,
    .init_once = memkind_hugetlb_init_once
};

int memkind_hugetlb_get_mmap_flags(struct memkind *kind, int *flags)
{
    *flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_2MB;
    return 0;
}

void memkind_hugetlb_init_once(void)
{
    int err = memkind_arena_create_map(MEMKIND_HUGETLB);
    assert(err == 0);
}

