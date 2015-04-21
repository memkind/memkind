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

#include <stdio.h>
#include <errno.h>
#include <numa.h>

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
    .check_available = memkind_hugetlb_check_available_2mb,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_arena = memkind_thread_get_arena,
    .get_size = memkind_default_get_size,
    .init_once = memkind_hugetlb_init_once
};

static int memkind_hugetlb_check_available(struct memkind *kind, size_t huge_size);

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


int memkind_hugetlb_check_available_2mb(struct memkind *kind)
{
    return memkind_hugetlb_check_available(kind, 2097152);
}

int memkind_hugetlb_check_available_1gb(struct memkind *kind)
{
    return memkind_hugetlb_check_available(kind, 1073741824);
}

/* huge_size: the huge page size in bytes */
static int memkind_hugetlb_check_available(struct memkind *kind, size_t huge_size)
{
    int err = 0;
    int errno_before;
    int num_read = 0;
    unsigned int node, num_node;
    size_t nr_hugepages = 0;
    FILE *fid = NULL;
    nodemask_t nodemask;
    struct bitmask nodemask_bm = {NUMA_NUM_NODES, nodemask.n};
    const char *nr_path_fmt = "/sys/devices/system/node/node%u/hugepages/hugepages-%zukB/nr_hugepages";
    char nr_path[128];

    /* default huge page size is 2MB */
    if (huge_size == 0) {
        huge_size = 2048;
    }
    /* convert input to kB */
    else {
        huge_size = huge_size >> 10;
    }

    if (kind->ops->get_mbind_nodemask) {
        err = kind->ops->get_mbind_nodemask(kind, nodemask.n, NUMA_NUM_NODES);
    }
    else {
        numa_bitmask_setall(&nodemask_bm);
    }
    num_node = numa_num_configured_nodes();
    for (node = 0; !err && node < num_node; ++node) {
        if (numa_bitmask_isbitset(&nodemask_bm, node)) {
            snprintf(nr_path, 128, nr_path_fmt, node, huge_size);
            errno_before = errno;
            fid = fopen(nr_path, "r");
            if (!fid) {
                err = MEMKIND_ERROR_HUGETLB;
                errno = errno_before;
            }
            else {
                num_read = fscanf(fid, "%zud", &nr_hugepages);
                fclose(fid);
                if (!num_read || !nr_hugepages) {
                    err = MEMKIND_ERROR_HUGETLB;
                }
            }
        }
    }
    return err;
}
