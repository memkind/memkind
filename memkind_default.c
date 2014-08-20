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

#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>
#include <jemalloc/jemalloc.h>
#include "memkind_default.h"


int memkind_default_create(struct memkind *kind, const struct memkind_ops *ops, const char *name)
{
    int err = 0;

    kind->ops = ops;
    if (strlen(name) >= MEMKIND_NAME_LENGTH) {
        err = MEMKIND_ERROR_INVALID;
    }
    if (!err) {
        strcpy(kind->name, name);
    }
    return err;
}

int memkind_default_destroy(struct memkind *kind)
{
    return 0;
}

void *memkind_default_malloc(struct memkind *kind, size_t size)
{
    return je_malloc(size);
}

void *memkind_default_calloc(struct memkind *kind, size_t num, size_t size)
{
    return je_calloc(num, size);
}

int memkind_default_posix_memalign(struct memkind *kind, void **memptr, size_t alignment, size_t size)
{
    return je_posix_memalign(memptr, alignment, size);
}

void *memkind_default_realloc(struct memkind *kind, void *ptr, size_t size)
{
    return je_realloc(ptr, size);
}

void memkind_default_free(struct memkind *kind, void *ptr)
{
    je_free(ptr);
}

int memkind_default_is_available(struct memkind *kind)
{
    return 1;
}

int memkind_default_mbind(struct memkind *kind, void *ptr, size_t len)
{
    nodemask_t nodemask;
    int err = 0;
    int mode;

    err = kind->ops->get_mbind_nodemask(kind, nodemask.n, NUMA_NUM_NODES);
    if (!err) {
        err = kind->ops->get_mbind_mode(kind, &mode);
    }
    if (!err) {
        err = mbind(ptr, len, mode, nodemask.n, NUMA_NUM_NODES, 0);
        err = err ? MEMKIND_ERROR_MBIND : 0;
    }
    return err;
}

int memkind_noop_mbind(struct memkind *kind, void *ptr, size_t len)
{
    return 0;
}

int memkind_default_get_mmap_flags(struct memkind *kind, int *flags)
{
    *flags = MAP_PRIVATE | MAP_ANONYMOUS;
    return 0;
}

int memkind_default_get_mbind_mode(struct memkind *kind, int *mode)
{
    *mode = MPOL_BIND;
    return 0;
}

int memkind_default_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};
    numa_bitmask_clearall(&nodemask_bm);
    numa_bitmask_setbit(&nodemask_bm, numa_preferred());
    return 0;
}

int memkind_default_get_size(struct memkind *kind, size_t *total, size_t *free)
{
    nodemask_t nodemask;
    struct bitmask nodemask_bm = {NUMA_NUM_NODES, nodemask.n};
    long long f;
    int err = 0;
    int i;

    *total = 0;
    *free = 0;
    if (kind->ops->get_mbind_nodemask) {
        err = kind->ops->get_mbind_nodemask(kind, nodemask.n, NUMA_NUM_NODES);
    }
    else {
        copy_bitmask_to_bitmask(numa_all_nodes_ptr, &nodemask_bm);
    }
    if (!err) {
        for (i = 0; i < NUMA_NUM_NODES; ++i) {
            if (numa_bitmask_isbitset(&nodemask_bm, i)) {
                *total += numa_node_size64(i, &f);
                *free += f;
            }
        }
    }
    return err;
}

