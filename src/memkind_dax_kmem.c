// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2021 Intel Corporation. */

#include <memkind/internal/heap_manager.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_bitmask.h>
#include <memkind/internal/memkind_dax_kmem.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_log.h>

#include "config.h"
#include <errno.h>
#include <numa.h>
#include <pthread.h>

struct dax_numanode_t {
    int init_err;
    void *numanode;
};

static struct dax_numanode_t memkind_dax_kmem_numanode_g[NODE_VARIANT_MAX_EXT];
static pthread_once_t memkind_dax_kmem_numanode_once_g[NODE_VARIANT_MAX_EXT] = {
    PTHREAD_ONCE_INIT};

static void memkind_dax_kmem_closest_numanode_init(void);
static void memkind_dax_kmem_preferred_closest_numanode_init(void);
static void memkind_dax_kmem_all_numanode_init(void);
#ifdef MEMKIND_DAXCTL_KMEM
#include <daxctl/libdaxctl.h>
#ifndef daxctl_region_foreach_safe
#define daxctl_region_foreach_safe(ctx, region, _region)                       \
    for (region = daxctl_region_get_first(ctx),                                \
        _region = region ? daxctl_region_get_next(region) : NULL;              \
         region != NULL; region = _region,                                     \
        _region = _region ? daxctl_region_get_next(_region) : NULL)
#endif

#ifndef daxctl_dev_foreach_safe
#define daxctl_dev_foreach_safe(region, dev, _dev)                             \
    for (dev = daxctl_dev_get_first(region),                                   \
        _dev = dev ? daxctl_dev_get_next(dev) : NULL;                          \
         dev != NULL;                                                          \
         dev = _dev, _dev = _dev ? daxctl_dev_get_next(_dev) : NULL)
#endif

static int get_dax_kmem_nodemask(struct bitmask **dax_kmem_node_mask)
{
    struct daxctl_region *region, *_region;
    struct daxctl_dev *dev, *_dev;
    struct daxctl_ctx *ctx;
    // NUMA Nodes could be not in arithmetic progression
    int nodes_num = numa_max_node() + 1;
    *dax_kmem_node_mask = numa_bitmask_alloc(nodes_num);
    if (*dax_kmem_node_mask == NULL) {
        log_err("numa_bitmask_alloc() failed.");
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    int rc = daxctl_new(&ctx);
    if (rc < 0)
        goto free_mask;

    daxctl_region_foreach_safe(ctx, region, _region)
    {
        daxctl_dev_foreach_safe(region, dev, _dev)
        {
            struct daxctl_memory *mem = daxctl_dev_get_memory(dev);
            if (mem) {
                numa_bitmask_setbit(*dax_kmem_node_mask,
                                    (unsigned)daxctl_dev_get_target_node(dev));
            }
        }
    }

    daxctl_unref(ctx);

    if (numa_bitmask_weight(*dax_kmem_node_mask) != 0) {
        return MEMKIND_SUCCESS;
    }

free_mask:
    numa_bitmask_free(*dax_kmem_node_mask);
    return MEMKIND_ERROR_UNAVAILABLE;
}
#else
static int get_dax_kmem_nodemask(struct bitmask **bm)
{
    log_err("DAX KMEM nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
}
#endif

static int memkind_dax_kmem_get_nodemask(struct bitmask **bm)
{
    char *nodes_env = memkind_get_env("MEMKIND_DAX_KMEM_NODES");
    if (nodes_env) {
        return memkind_env_get_nodemask(nodes_env, bm);
    } else {
        return get_dax_kmem_nodemask(bm);
    }
}

static int memkind_dax_kmem_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

static int memkind_dax_kmem_get_mbind_nodemask(struct memkind *kind,
                                               unsigned long *nodemask,
                                               unsigned long maxnode)
{
    struct dax_numanode_t *g =
        &memkind_dax_kmem_numanode_g[NODE_VARIANT_MULTIPLE];
    pthread_once(&memkind_dax_kmem_numanode_once_g[NODE_VARIANT_MULTIPLE],
                 memkind_dax_kmem_closest_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err)) {
        g->init_err =
            set_bitmask_for_current_numanode(nodemask, maxnode, g->numanode);
    }
    return g->init_err;
}

static int memkind_dax_kmem_get_preferred_mbind_nodemask(
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct dax_numanode_t *g =
        &memkind_dax_kmem_numanode_g[NODE_VARIANT_SINGLE];
    pthread_once(&memkind_dax_kmem_numanode_once_g[NODE_VARIANT_SINGLE],
                 memkind_dax_kmem_preferred_closest_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err)) {
        g->init_err =
            set_bitmask_for_current_numanode(nodemask, maxnode, g->numanode);
    }
    return g->init_err;
}

MEMKIND_EXPORT int memkind_dax_kmem_all_get_mbind_nodemask(
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct dax_numanode_t *g = &memkind_dax_kmem_numanode_g[NODE_VARIANT_ALL];
    pthread_once(&memkind_dax_kmem_numanode_once_g[NODE_VARIANT_ALL],
                 memkind_dax_kmem_all_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err)) {
        g->init_err =
            set_bitmask_for_current_numanode(nodemask, maxnode, g->numanode);
    }
    return g->init_err;
}

static void memkind_dax_kmem_closest_numanode_init(void)
{
    struct dax_numanode_t *g =
        &memkind_dax_kmem_numanode_g[NODE_VARIANT_MULTIPLE];
    g->numanode = NULL;
    g->init_err = set_closest_numanode(memkind_dax_kmem_get_nodemask,
                                       &g->numanode, NODE_VARIANT_MULTIPLE);
}

static void memkind_dax_kmem_preferred_closest_numanode_init(void)
{
    struct dax_numanode_t *g =
        &memkind_dax_kmem_numanode_g[NODE_VARIANT_SINGLE];
    g->numanode = NULL;
    g->init_err = set_closest_numanode(memkind_dax_kmem_get_nodemask,
                                       &g->numanode, NODE_VARIANT_SINGLE);
}

static void memkind_dax_kmem_all_numanode_init(void)
{
    struct dax_numanode_t *g = &memkind_dax_kmem_numanode_g[NODE_VARIANT_ALL];
    g->numanode = NULL;
    g->init_err = set_closest_numanode(memkind_dax_kmem_get_nodemask,
                                       &g->numanode, NODE_VARIANT_ALL);
}

static void memkind_dax_kmem_init_once(void)
{
    memkind_init(MEMKIND_DAX_KMEM, true);
}

static void memkind_dax_kmem_all_init_once(void)
{
    memkind_init(MEMKIND_DAX_KMEM_ALL, true);
}

static void memkind_dax_kmem_preferred_init_once(void)
{
    memkind_init(MEMKIND_DAX_KMEM_PREFERRED, true);
}

static void memkind_dax_kmem_interleave_init_once(void)
{
    memkind_init(MEMKIND_DAX_KMEM_INTERLEAVE, true);
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
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_DAX_KMEM_ALL_OPS = {
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
    .get_mbind_nodemask = memkind_dax_kmem_all_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_dax_kmem_all_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

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
    .get_mbind_nodemask = memkind_dax_kmem_get_preferred_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_dax_kmem_preferred_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_DAX_KMEM_INTERLEAVE_OPS = {
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
    .get_mbind_mode = memkind_interleave_get_mbind_mode,
    .get_mbind_nodemask = memkind_dax_kmem_all_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_dax_kmem_interleave_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};
