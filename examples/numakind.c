/*
 * Copyright (C) 2014 Intel Corporation.
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

/*
 * In this example we try to create an optimal allocator for use on a
 * NUMA system with threading.  We assume that by the time a thread
 * makes an allocation call it is being run on the CPU that it will
 * remain on for the thread lifetime.  If this is not the case, and
 * the thread is migrated between allocation calls, there is no way
 * to insure that the thread's data is local anyway.
 *
 * We will create one heap partition for each NUMA node to which the
 * partition will be bound.  There will be one arena per CPU, and the
 * index for the arena will be kept in thread local storage.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <assert.h>
#include <numa.h>
#include <errno.h>
#include <jemalloc/jemalloc.h>
#include <memkind.h>
#include <memkind_default.h>
#include <memkind_arena.h>

static pthread_key_t numakind_key_g;
static pthread_once_t numakind_init_once_g = PTHREAD_ONCE_INIT;
static int numakind_init_err_g = 0;
static int numakind_zero_partition_g;

#ifndef NUMAKIND_MAX
#define NUMAKIND_MAX 2048
#endif

#define NUMAKIND_GET_MBIND_NODEMASK_MACRO(NODE)                   \
int get_mbind_nodemask_numa_##NODE(struct memkind *kind,          \
    unsigned long *nodemask, unsigned long maxnode)               \
{                                                                 \
    unsigned int node_id = NODE;                                  \
    struct bitmask nodemask_bm = {maxnode, nodemask};             \
    numa_bitmask_clearall(&nodemask_bm);                          \
    numa_bitmask_setbit(&nodemask_bm, node_id);                   \
    return 0;                                                     \
}

#define NUMAKIND_OPS_MACRO(NODE)                                  \
{                                                                 \
    .create = memkind_arena_create,                               \
    .destroy = memkind_arena_destroy,                             \
    .malloc = memkind_arena_malloc,                               \
    .calloc = memkind_arena_calloc,                               \
    .posix_memalign = memkind_arena_posix_memalign,               \
    .realloc = memkind_arena_realloc,                             \
    .free = memkind_default_free,                                 \
    .get_arena = memkind_thread_get_arena,                        \
    .get_size = memkind_default_get_size,                         \
    .get_mbind_nodemask = get_mbind_nodemask_numa_##NODE          \
}

#include "numakind_macro.h"

static void numakind_init(void);
static int numakind_get_kind(memkind_t **kind);

void *numakind_malloc(size_t size)
{
    int err = 0;
    void *result = NULL;
    memkind_t *kind = NULL;

    err = numakind_get_kind(&kind);
    if (!err) {
        result = (void *)memkind_malloc(*kind, size);
    }
    return result;
}

void *numakind_calloc(size_t num, size_t size)
{
    int err = 0;
    void *result = NULL;
    memkind_t *kind = NULL;

    err = numakind_get_kind(&kind);
    if (!err) {
        result = (void *)memkind_calloc(*kind, num, size);
    }
    return result;
}

int numakind_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    int err = 0;
    memkind_t *kind = NULL;

    err = numakind_get_kind(&kind) ? EINVAL : 0;
    if (!err) {
        err = memkind_posix_memalign(*kind, memptr, alignment, size);
    }
    return err;
}

void *numakind_realloc(void *ptr, size_t size)
{
    int err = 0;
    void *result = NULL;
    memkind_t *kind = NULL;

    err = numakind_get_kind(&kind);
    if (!err) {
        result = (void *)memkind_realloc(*kind, ptr, size);
    }
    return result;
}

void numakind_free(void *ptr)
{
    memkind_default_free(MEMKIND_DEFAULT, ptr);
}

static void numakind_init(void)
{
    int err = 0;
    char numakind_name[MEMKIND_NAME_LENGTH];
    int i, name_length, num_nodes;
    memkind_t numakind;

    pthread_key_create(&numakind_key_g, je_free);

    num_nodes = numa_num_configured_nodes();
    if (num_nodes > NUMAKIND_MAX) {
        err = MEMKIND_ERROR_TOOMANY;
    }

    for (i = 0; !err && i < num_nodes; ++i) {
        name_length = snprintf(numakind_name, MEMKIND_NAME_LENGTH - 1, "numakind_%.4d", i);
        assert(MEMKIND_NAME_LENGTH > 20 && name_length != MEMKIND_NAME_LENGTH - 1);

        err = memkind_create(NUMAKIND_OPS + i, numakind_name, &numakind);
        if (!err) {
            if (i == 0) {
                numakind_zero_partition_g = numakind->partition;
            }
            else if (numakind->partition != numakind_zero_partition_g + i) {
                err = -1;
            }
        }
    }
    if (err) {
        numakind_init_err_g = err;
    }
}

static int numakind_get_kind(memkind_t **kind)
{
    int err = 0;
    unsigned int thread_numa_node;

    pthread_once(&numakind_init_once_g, numakind_init);
    if (numakind_init_err_g) {
        err = numakind_init_err_g;
    }

    if (!err) {
        *kind = pthread_getspecific(numakind_key_g);
        if (*kind == NULL) {
            *kind = (memkind_t *)je_malloc(sizeof(memkind_t));
            if (*kind == NULL) {
                err = MEMKIND_ERROR_MALLOC;
            }
            if (!err) {
                thread_numa_node = numa_node_of_cpu(sched_getcpu());
                err = memkind_get_kind_by_partition(numakind_zero_partition_g + thread_numa_node, *kind);
            }
            if (!err) {
                err = pthread_setspecific(numakind_key_g, *kind) ?
                      MEMKIND_ERROR_PTHREAD : 0;
            }
        }
    }
    return err;
}
