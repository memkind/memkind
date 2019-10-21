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

#include "config.h"
#include <pthread.h>
#include <numa.h>
#include <errno.h>

struct dax_closest_numanode_t {
    int init_err;
    int num_cpu;
    int *closest_numanode;
};

static struct dax_closest_numanode_t memkind_dax_kmem_closest_numanode_g;
static pthread_once_t memkind_dax_kmem_closest_numanode_once_g =
    PTHREAD_ONCE_INIT;

static void memkind_dax_kmem_closest_numanode_init(void);

#ifdef MEMKIND_DAXCTL_KMEM
#include <daxctl/libdaxctl.h>
#ifndef daxctl_region_foreach_safe
#define daxctl_region_foreach_safe(ctx, region, _region) \
    for (region = daxctl_region_get_first(ctx), \
         _region = region ? daxctl_region_get_next(region) : NULL; \
         region != NULL; \
         region = _region, \
        _region = _region ? daxctl_region_get_next(_region) : NULL)
#endif

#ifndef daxctl_dev_foreach_safe
#define daxctl_dev_foreach_safe(region, dev, _dev) \
    for (dev = daxctl_dev_get_first(region), \
         _dev = dev ? daxctl_dev_get_next(dev) : NULL; \
         dev != NULL; \
         dev = _dev, \
        _dev = _dev ? daxctl_dev_get_next(_dev) : NULL)
#endif

static int get_dax_kmem_nodes(struct bitmask *dax_kmem_node_mask)
{
    struct daxctl_region *region, *_region;
    struct daxctl_dev *dev, *_dev;
    struct daxctl_ctx *ctx;

    int rc = daxctl_new(&ctx);
    if (rc < 0)
        return MEMKIND_ERROR_UNAVAILABLE;

    daxctl_region_foreach_safe(ctx, region, _region) {
        daxctl_dev_foreach_safe(region, dev, _dev) {
            struct daxctl_memory *mem = daxctl_dev_get_memory(dev);
            if (mem) {
                numa_bitmask_setbit(dax_kmem_node_mask,
                                    (unsigned)daxctl_dev_get_target_node(dev));
            }
        }
    }

    daxctl_unref(ctx);

    return (numa_bitmask_weight(dax_kmem_node_mask) != 0) ? MEMKIND_SUCCESS :
           MEMKIND_ERROR_UNAVAILABLE;
}

static int fill_dax_kmem_values_automatic(int *bandwidth)
{
    return bandwidth_fill(bandwidth, get_dax_kmem_nodes);
}
#else
static int fill_dax_kmem_values_automatic(int *bandwidth)
{
    log_err("DAX KMEM nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
}
#endif

static int memkind_dax_kmem_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

static int memkind_dax_kmem_get_mbind_nodemask(struct memkind *kind,
                                               unsigned long *nodemask, unsigned long maxnode)
{
    struct dax_closest_numanode_t *g = &memkind_dax_kmem_closest_numanode_g;
    pthread_once(&memkind_dax_kmem_closest_numanode_once_g,
                 memkind_dax_kmem_closest_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err)) {
        g->init_err = set_bitmask_for_current_closest_numanode(nodemask, maxnode,
                                                               g->closest_numanode, g->num_cpu);
    }
    return g->init_err;
}

MEMKIND_EXPORT int memkind_dax_kmem_all_get_mbind_nodemask(struct memkind *kind,
                                                           unsigned long *nodemask, unsigned long maxnode)
{
    struct dax_closest_numanode_t *g = &memkind_dax_kmem_closest_numanode_g;
    pthread_once(&memkind_dax_kmem_closest_numanode_once_g,
                 memkind_dax_kmem_closest_numanode_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        set_bitmask_for_all_closest_numanodes(nodemask, maxnode, g->closest_numanode,
                                              g->num_cpu);
    }
    return g->init_err;
}

static void memkind_dax_kmem_closest_numanode_init(void)
{
    struct dax_closest_numanode_t *g = &memkind_dax_kmem_closest_numanode_g;
    g->num_cpu = numa_num_configured_cpus();
    g->closest_numanode = NULL;
    g->init_err = set_closest_numanode(fill_dax_kmem_values_automatic,
                                       "MEMKIND_DAX_KMEM_NODES", &g->closest_numanode, g->num_cpu);

}

static void memkind_dax_kmem_init_once(void)
{
    memkind_init(MEMKIND_DAX_KMEM, true);
}

static void memkind_dax_kmem_preferred_init_once(void)
{
    memkind_init(MEMKIND_DAX_KMEM_PREFERRED, true);
}

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

MEMKIND_EXPORT struct memkind_ops MEMKIND_DAX_KMEM_PREFERRED_OPS = {
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
    .get_mbind_mode = memkind_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_dax_kmem_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_dax_kmem_preferred_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize
};
