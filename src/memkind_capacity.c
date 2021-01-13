// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_mem_attributes.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/memkind_log.h>

#include <numa.h>

struct hi_cap_loc_numanodes_t {
    int init_err;
    struct bitmask **per_cpu_numa_nodes;
};

struct hi_cap_numanodes_t {
    int init_err;
    struct bitmask *numa_nodes;
};

static struct hi_cap_loc_numanodes_t
    memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_MAX];
static pthread_once_t memkind_hi_cap_loc_numanodes_once_g[NODE_VARIANT_MAX] = {PTHREAD_ONCE_INIT};

static struct hi_cap_numanodes_t memkind_hi_cap_numanodes_g[NODE_VARIANT_MAX];
static pthread_once_t memkind_hi_cap_numanodes_once_g[NODE_VARIANT_MAX] = {PTHREAD_ONCE_INIT};

static void memkind_hi_cap_loc_numanodes_init(void)
{
    struct hi_cap_loc_numanodes_t *g =
            &memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_MULTIPLE];
    g->init_err = get_per_cpu_hi_cap_local_nodes_mask(&g->per_cpu_numa_nodes,
                                                      NODE_VARIANT_MULTIPLE);
}

static void memkind_hi_cap_loc_preferred_numanodes_init(void)
{
    struct hi_cap_loc_numanodes_t *g =
            &memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_SINGLE];
    g->init_err = get_per_cpu_hi_cap_local_nodes_mask(&g->per_cpu_numa_nodes,
                                                      NODE_VARIANT_SINGLE);
}

static int memkind_hi_cap_loc_get_mbind_nodemask(
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct hi_cap_loc_numanodes_t *g =
            &memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_MULTIPLE];
    pthread_once(&memkind_hi_cap_loc_numanodes_once_g[NODE_VARIANT_MULTIPLE],
                 memkind_hi_cap_loc_numanodes_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        int cpu_id = sched_getcpu();
        struct bitmask nodemask_bm = {maxnode, nodemask};
        copy_bitmask_to_bitmask(g->per_cpu_numa_nodes[cpu_id], &nodemask_bm);
    }

    return g->init_err;
}

static int memkind_hi_cap_loc_preferred_get_mbind_nodemask(
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct hi_cap_loc_numanodes_t *g =
            &memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_SINGLE];

    pthread_once(&memkind_hi_cap_loc_numanodes_once_g[NODE_VARIANT_SINGLE],
                 memkind_hi_cap_loc_preferred_numanodes_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        int cpu_id = sched_getcpu();
        struct bitmask nodemask_bm = {maxnode, nodemask};
        copy_bitmask_to_bitmask(g->per_cpu_numa_nodes[cpu_id], &nodemask_bm);
    }

    return g->init_err;
}

static int memkind_hi_cap_loc_finalize(memkind_t kind)
{
    struct hi_cap_loc_numanodes_t *g =
            &memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_SINGLE];

    if (MEMKIND_LIKELY(!g->init_err)) {
        if (g->per_cpu_numa_nodes) {
            int num_cpus = numa_num_configured_cpus();
            int i;
            for (i = 0; i < num_cpus; ++i) {
                if (g->per_cpu_numa_nodes[i]) {
                    numa_bitmask_free(g->per_cpu_numa_nodes[i]);
                }
            }
        }
        free(g->per_cpu_numa_nodes);
    }

    return memkind_arena_finalize(kind);
}

static int memkind_hi_cap_loc_preferred_finalize(memkind_t kind)
{
    struct hi_cap_loc_numanodes_t *g =
            &memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_MULTIPLE];

    if (MEMKIND_LIKELY(!g->init_err)) {
        if (g->per_cpu_numa_nodes) {
            int num_cpus = numa_num_configured_cpus();
            int i;
            for (i = 0; i < num_cpus; ++i) {
                if (g->per_cpu_numa_nodes[i]) {
                    numa_bitmask_free(g->per_cpu_numa_nodes[i]);
                }
            }
        }
        free(g->per_cpu_numa_nodes);
    }

    return memkind_arena_finalize(kind);
}

static int memkind_hi_cap_loc_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

static void memkind_hi_cap_loc_init_once(void)
{
    memkind_init(MEMKIND_HIGHEST_CAPACITY_LOCAL, true);
}

static void memkind_hi_cap_loc_preferred_init_once(void)
{
    memkind_init(MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED, true);
}

static void memkind_hi_cap_find(struct hi_cap_numanodes_t *g, int node_variant)
{
    long long best = 0;
    int max_node_id = numa_max_node();
    int node;
    g->numa_nodes = numa_bitmask_alloc(max_node_id + 1);

    if (MEMKIND_UNLIKELY(g->numa_nodes == NULL)) {
        g->init_err = MEMKIND_ERROR_MALLOC;
        log_err("numa_bitmask_alloc() failed.");
        return;
    }

    for (node = 0; node <= max_node_id; ++node) {
        long long current = numa_node_size64(node, NULL);

        if (current == -1) {
            continue;
        }

        if (current > best) {
            best = current;
            numa_bitmask_clearall(g->numa_nodes);
        }

        if (current == best) {
            numa_bitmask_setbit(g->numa_nodes, node);
        }
    }

    if (numa_bitmask_weight(g->numa_nodes) == 0) {
        numa_bitmask_free(g->numa_nodes);
        g->init_err = MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
        return;
    }

    if (node_variant == NODE_VARIANT_SINGLE &&
        numa_bitmask_weight(g->numa_nodes) > 1) {
        numa_bitmask_free(g->numa_nodes);
        log_err("Multiple NUMA Nodes have the same highest capacity.");
        g->init_err = MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
        return;
    }

    g->init_err = MEMKIND_SUCCESS;
}

static void memkind_hi_cap_numanodes_init(void)
{
    memkind_hi_cap_find(&memkind_hi_cap_numanodes_g[NODE_VARIANT_MULTIPLE],
                        NODE_VARIANT_MULTIPLE);
}

static void memkind_hi_cap_preferred_numanodes_init(void)
{
    memkind_hi_cap_find(&memkind_hi_cap_numanodes_g[NODE_VARIANT_SINGLE],
                        NODE_VARIANT_SINGLE);
}

static int memkind_hi_cap_get_mbind_nodemask(struct memkind *kind,
                                             unsigned long *nodemask, unsigned long maxnode)
{
    struct hi_cap_numanodes_t *g =
            &memkind_hi_cap_numanodes_g[NODE_VARIANT_MULTIPLE];
    pthread_once(&memkind_hi_cap_numanodes_once_g[NODE_VARIANT_MULTIPLE],
                 memkind_hi_cap_numanodes_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        struct bitmask nodemask_bm = {maxnode, nodemask};
        copy_bitmask_to_bitmask(g->numa_nodes, &nodemask_bm);
    }

    return g->init_err;
}

static int memkind_hi_cap_preferred_get_mbind_nodemask(
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct hi_cap_numanodes_t *g = &memkind_hi_cap_numanodes_g[NODE_VARIANT_SINGLE];
    pthread_once(&memkind_hi_cap_numanodes_once_g[NODE_VARIANT_SINGLE],
                 memkind_hi_cap_preferred_numanodes_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        struct bitmask nodemask_bm = {maxnode, nodemask};
        copy_bitmask_to_bitmask(g->numa_nodes, &nodemask_bm);
    }

    return g->init_err;
}

static int memkind_hi_cap_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

static void memkind_hi_cap_init_once(void)
{
    memkind_init(MEMKIND_HIGHEST_CAPACITY, true);
}

static void memkind_hi_cap_preferred_init_once(void)
{
    memkind_init(MEMKIND_HIGHEST_CAPACITY_PREFERRED, true);
}

static int memkind_hi_cap_finalize(memkind_t kind)
{
    struct hi_cap_numanodes_t *g = &memkind_hi_cap_numanodes_g[NODE_VARIANT_SINGLE];
    if(g->numa_nodes)
        numa_bitmask_free(g->numa_nodes);

    return memkind_arena_finalize(kind);
}

static int memkind_hi_cap_preferred_finalize(memkind_t kind)
{
    struct hi_cap_numanodes_t *g =
            &memkind_hi_cap_numanodes_g[NODE_VARIANT_MULTIPLE];
    if(g->numa_nodes)
        numa_bitmask_free(g->numa_nodes);

    return memkind_arena_finalize(kind);
}

MEMKIND_EXPORT struct memkind_ops MEMKIND_HIGHEST_CAPACITY_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hi_cap_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hi_cap_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hi_cap_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_hi_cap_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HIGHEST_CAPACITY_PREFERRED_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hi_cap_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_hi_cap_preferred_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hi_cap_preferred_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_hi_cap_preferred_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HIGHEST_CAPACITY_LOCAL_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hi_cap_loc_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hi_cap_loc_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hi_cap_loc_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_hi_cap_loc_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED_OPS
    = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hi_cap_loc_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_hi_cap_loc_preferred_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hi_cap_loc_preferred_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_hi_cap_loc_preferred_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate
};