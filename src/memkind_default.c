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

#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>
#include <errno.h>
#include <jemalloc/jemalloc.h>

#include "memkind_default.h"

const struct memkind_ops MEMKIND_DEFAULT_OPS = {
    .create = memkind_default_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_default_malloc,
    .calloc = memkind_default_calloc,
    .posix_memalign = memkind_default_posix_memalign,
    .realloc = memkind_default_realloc,
    .free = memkind_default_free,
    .get_size = memkind_default_get_size
};

int memkind_default_create(struct memkind *kind, const struct memkind_ops *ops, const char *name)
{
    int err = 0;

    kind->ops = ops;
    if (strlen(name) >= MEMKIND_NAME_LENGTH) {
        kind->name[0] = '\0';
        err = MEMKIND_ERROR_INVALID;
    }
    if (!err) {
        kind->name[MEMKIND_NAME_LENGTH-1] = '\0';
        strncpy(kind->name, name, MEMKIND_NAME_LENGTH-1);
    }
    return err;
}

int memkind_default_destroy(struct memkind *kind)
{
    kind->name[0] = '\0';
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

void *memkind_default_mmap(struct memkind *kind, void *addr, size_t size)
{
    void *result = MAP_FAILED;
    int err = 0;
    int flags;
    int fd;
    off_t offset;

    if (kind->ops->get_mmap_flags) {
        err = kind->ops->get_mmap_flags(kind, &flags);
    }
    else {
        err = memkind_default_get_mmap_flags(kind, &flags);
    }
    if (!err) {
        if (kind->ops->get_mmap_file) {
            err = kind->ops->get_mmap_file(kind, &fd, &offset);
        }
        else {
            err = memkind_default_get_mmap_file(kind, &fd, &offset);
        }
    }
    if (!err) {
        result = mmap(addr, size, PROT_READ | PROT_WRITE, flags, fd, offset);
    }
    if (result != MAP_FAILED && kind->ops->mbind) {
        err = kind->ops->mbind(kind, result, size);
        if (err) {
            munmap(result, size);
            result = MAP_FAILED;
        }
    }
    return result;
}

int memkind_default_mbind(struct memkind *kind, void *ptr, size_t size)
{
    nodemask_t nodemask;
    int err = 0;
    int mode;

    err = kind->ops->get_mbind_nodemask(kind, nodemask.n, NUMA_NUM_NODES);
    if (!err) {
        err = kind->ops->get_mbind_mode(kind, &mode);
    }
    if (!err) {
        err = mbind(ptr, size, mode, nodemask.n, NUMA_NUM_NODES, 0);
        err = err ? MEMKIND_ERROR_MBIND : 0;
    }
    return err;
}

int memkind_default_get_mmap_flags(struct memkind *kind, int *flags)
{
    *flags = MAP_PRIVATE | MAP_ANONYMOUS;
    return 0;
}

int memkind_default_get_mmap_file(struct memkind *kind, int *fd, off_t *offset)
{
    *fd = -1;
    *offset = 0;
    return 0;
}

int memkind_default_get_mbind_mode(struct memkind *kind, int *mode)
{
    *mode = MPOL_BIND;
    return 0;
}

int memkind_preferred_get_mbind_mode(struct memkind *kind, int *mode)
{
    *mode = MPOL_PREFERRED;
    return 0;
}

int memkind_posix_check_alignment(struct memkind *kind, size_t alignment)
{
    int err = 0;
    if ((alignment < sizeof(void*)) ||
        (((alignment - 1) & alignment) != 0)) {
        err = EINVAL;
    }
    return err;
}
