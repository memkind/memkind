/*
 * Copyright (C) 2019 Intel Corporation.
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

#include <memkind/internal/memkind_bandwidth.h>
#include <memkind/internal/memkind_dax_kmem.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/heap_manager.h>

#include <pthread.h>
#include <numa.h>
#include <errno.h>
#include <jemalloc/jemalloc.h>

MEMKIND_EXPORT struct memkind_ops MEMKIND_DAX_KMEM_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_dax_kmem_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_dax_kmem_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_dax_kmem_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize
};

static struct bandwidth_closest_numanode_t memkind_dax_kmem_closest_numanode_g;
static pthread_once_t memkind_dax_kmem_closest_numanode_once_g =
    PTHREAD_ONCE_INIT;

static void memkind_dax_kmem_closest_numanode_init(void);

static int fill_kmem_bandwidth_values(int *bandwidth)
{
// TODO(kfilipek): detect DAX KMEM nodes
    log_err("DAX KMEM nodes cannot be automatically detected.");
    return MEMKIND_ERROR_UNAVAILABLE;
}

MEMKIND_EXPORT int memkind_dax_kmem_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

MEMKIND_EXPORT int memkind_dax_kmem_get_mbind_nodemask(struct memkind *kind,
                                                       unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};
    struct bandwidth_closest_numanode_t *g = &memkind_dax_kmem_closest_numanode_g;
    pthread_once(&memkind_dax_kmem_closest_numanode_once_g,
                 memkind_dax_kmem_closest_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err && nodemask)) {
        numa_bitmask_clearall(&nodemask_bm);
        int cpu = sched_getcpu();
        if (MEMKIND_LIKELY(cpu < g->num_cpu)) {
            numa_bitmask_setbit(&nodemask_bm, g->closest_numanode[cpu]);
        } else {
            return MEMKIND_ERROR_RUNTIME;
        }
    }
    return g->init_err;
}

MEMKIND_EXPORT int memkind_dax_kmem_all_get_mbind_nodemask(struct memkind *kind,
                                                           unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};
    struct bandwidth_closest_numanode_t *g = &memkind_dax_kmem_closest_numanode_g;
    pthread_once(&memkind_dax_kmem_closest_numanode_once_g,
                 memkind_dax_kmem_closest_numanode_init);

    if (MEMKIND_LIKELY(!g->init_err && nodemask)) {
        int cpu;
        numa_bitmask_clearall(&nodemask_bm);
        for (cpu = 0; cpu < g->num_cpu; ++cpu) {
            numa_bitmask_setbit(&nodemask_bm, g->closest_numanode[cpu]);
        }
    }
    return g->init_err;
}

static void memkind_dax_kmem_closest_numanode_init(void)
{
    struct bandwidth_closest_numanode_t *g = &memkind_dax_kmem_closest_numanode_g;
    int *bandwidth;
    int num_unique = 0;

    struct bandwidth_nodes_t *bandwidth_nodes = NULL;

    g->num_cpu = numa_num_configured_cpus();
    g->closest_numanode = (int *)jemk_malloc(sizeof(int) * g->num_cpu);
    bandwidth = (int *)jemk_malloc(sizeof(int) * NUMA_NUM_NODES);

    if (!(g->closest_numanode && bandwidth)) {
        g->init_err = MEMKIND_ERROR_MALLOC;
        log_err("jemk_malloc() failed.");
        goto exit;
    }

    g->init_err = bandwidth_fill_nodes(bandwidth, fill_kmem_bandwidth_values,
                                       "MEMKIND_DAX_KMEM_NODES");
    if (g->init_err)
        goto exit;

    g->init_err = bandwidth_create_nodes(NUMA_NUM_NODES, bandwidth, &num_unique,
                                         &bandwidth_nodes);
    if (g->init_err)
        goto exit;

    g->init_err = bandwidth_set_closest_numanode(num_unique, bandwidth_nodes,
                                                 g->num_cpu, g->closest_numanode);

exit:

    jemk_free(bandwidth_nodes);
    jemk_free(bandwidth);

    if (g->init_err) {
        jemk_free(g->closest_numanode);
        g->closest_numanode = NULL;
    }
}

MEMKIND_EXPORT void memkind_dax_kmem_init_once(void)
{
    memkind_init(MEMKIND_DAX_KMEM, true);
}
