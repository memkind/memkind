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

#include <stdlib.h>
#include <stdio.h>
#include <numa.h>
#include <errno.h>

#include <memkind.h>
#include <memkind_default.h>
#include <memkind_arena.h>

int node0_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
int node1_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
int node2_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
int node3_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);

static const struct memkind_ops NODE0_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = node0_get_mbind_nodemask,
    .get_arena = memkind_bijective_get_arena,
    .get_size = memkind_default_get_size
};

static const struct memkind_ops NODE1_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = node1_get_mbind_nodemask,
    .get_arena = memkind_bijective_get_arena,
    .get_size = memkind_default_get_size
};

static const struct memkind_ops NODE2_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = node2_get_mbind_nodemask,
    .get_arena = memkind_bijective_get_arena,
    .get_size = memkind_default_get_size
};

static const struct memkind_ops NODE3_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = node3_get_mbind_nodemask,
    .get_arena = memkind_bijective_get_arena,
    .get_size = memkind_default_get_size
};

int node0_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};
    numa_bitmask_clearall(&nodemask_bm);
    numa_bitmask_setbit(&nodemask_bm, 0);
    return 0;
}

int node1_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};
    numa_bitmask_clearall(&nodemask_bm);
    numa_bitmask_setbit(&nodemask_bm, 1);
    return 0;
}

int node2_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};
    numa_bitmask_clearall(&nodemask_bm);
    numa_bitmask_setbit(&nodemask_bm, 2);
    return 0;
}

int node3_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};
    numa_bitmask_clearall(&nodemask_bm);
    numa_bitmask_setbit(&nodemask_bm, 3);
    return 0;
}

int main(int argc, char **argv)
{
    const size_t SIZE = 1024;
    char err_msg[SIZE];
    memkind_t node_kind[4];
    char *str;
    int i, max, err;

    max = numa_num_configured_nodes();
    if (max > 4) {
        max = 4;
    }

    err = memkind_create(&NODE0_OPS, "node0", node_kind + 0);
    if (!err) {
        err = memkind_create(&NODE1_OPS, "node1", node_kind + 1);
    }
    if (!err) {
        err = memkind_create(&NODE2_OPS, "node2", node_kind + 2);
    }
    if (!err) {
        err = memkind_create(&NODE3_OPS, "node3", node_kind + 3);
    }
    if (err) {
        memkind_error_message(err, err_msg, SIZE);
        fprintf(stderr, err_msg);
        return 1;
    }

    for (i = 0; i < max; ++i) {
        str = (char *)memkind_malloc(node_kind[i], SIZE);
        if (str == NULL) {
            perror("memkind_malloc()");
            fprintf(stderr, "Unable to allocate string on node %d\n", i);
            return errno ? -errno : 1;
        }
        sprintf(str, "Hello from node %d\n", i);
        fprintf(stdout, str);
        memkind_free(node_kind[i], str);
    }

    memkind_finalize();
    return 0;
}
