// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_mem_attributes.h>
#include <memkind/internal/memkind_private.h>

struct loc_numanodes_t {
    int init_err;
    struct bitmask **per_cpu_numa_nodes;
};

static inline void local_kind_finalize(const struct loc_numanodes_t *g)
{
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
}

static struct loc_numanodes_t memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_MAX];
static pthread_once_t memkind_hi_cap_loc_numanodes_once_g[NODE_VARIANT_MAX] = {
    PTHREAD_ONCE_INIT};
static struct loc_numanodes_t memkind_low_lat_loc_numanodes_g[NODE_VARIANT_MAX];
static pthread_once_t memkind_low_lat_loc_numanodes_once_g[NODE_VARIANT_MAX] = {
    PTHREAD_ONCE_INIT};
static struct loc_numanodes_t memkind_hi_bw_loc_numanodes_g[NODE_VARIANT_MAX];
static pthread_once_t memkind_hi_bw_loc_numanodes_once_g[NODE_VARIANT_MAX] = {
    PTHREAD_ONCE_INIT};

static void memkind_hi_cap_loc_numanodes_init(void)
{
    struct loc_numanodes_t *g =
        &memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_MULTIPLE];
    g->init_err = get_per_cpu_local_nodes_mask(
        &g->per_cpu_numa_nodes, NODE_VARIANT_MULTIPLE, MEM_ATTR_CAPACITY);
}

static void memkind_hi_cap_loc_preferred_numanodes_init(void)
{
    struct loc_numanodes_t *g =
        &memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_SINGLE];
    g->init_err = get_per_cpu_local_nodes_mask(
        &g->per_cpu_numa_nodes, NODE_VARIANT_SINGLE, MEM_ATTR_CAPACITY);
}

static void memkind_low_lat_loc_numanodes_init(void)
{
    struct loc_numanodes_t *g =
        &memkind_low_lat_loc_numanodes_g[NODE_VARIANT_MULTIPLE];
    g->init_err = get_per_cpu_local_nodes_mask(
        &g->per_cpu_numa_nodes, NODE_VARIANT_MULTIPLE, MEM_ATTR_LATENCY);
}

static void memkind_low_lat_loc_preferred_numanodes_init(void)
{
    struct loc_numanodes_t *g =
        &memkind_low_lat_loc_numanodes_g[NODE_VARIANT_SINGLE];
    g->init_err = get_per_cpu_local_nodes_mask(
        &g->per_cpu_numa_nodes, NODE_VARIANT_SINGLE, MEM_ATTR_LATENCY);
}

static void memkind_hi_bw_loc_numanodes_init(void)
{
    struct loc_numanodes_t *g =
        &memkind_hi_bw_loc_numanodes_g[NODE_VARIANT_MULTIPLE];
    g->init_err = get_per_cpu_local_nodes_mask(
        &g->per_cpu_numa_nodes, NODE_VARIANT_MULTIPLE, MEM_ATTR_BANDWIDTH);
}

static void memkind_hi_bw_loc_preferred_numanodes_init(void)
{
    struct loc_numanodes_t *g =
        &memkind_hi_bw_loc_numanodes_g[NODE_VARIANT_SINGLE];
    g->init_err = get_per_cpu_local_nodes_mask(
        &g->per_cpu_numa_nodes, NODE_VARIANT_SINGLE, MEM_ATTR_BANDWIDTH);
}

static int memkind_hi_cap_loc_get_mbind_nodemask(struct memkind *kind,
                                                 unsigned long *nodemask,
                                                 unsigned long maxnode)
{
    struct loc_numanodes_t *g =
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
    struct loc_numanodes_t *g =
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

static int memkind_low_lat_loc_get_mbind_nodemask(struct memkind *kind,
                                                  unsigned long *nodemask,
                                                  unsigned long maxnode)
{
    struct loc_numanodes_t *g =
        &memkind_low_lat_loc_numanodes_g[NODE_VARIANT_MULTIPLE];
    pthread_once(&memkind_low_lat_loc_numanodes_once_g[NODE_VARIANT_MULTIPLE],
                 memkind_low_lat_loc_numanodes_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        int cpu_id = sched_getcpu();
        struct bitmask nodemask_bm = {maxnode, nodemask};
        copy_bitmask_to_bitmask(g->per_cpu_numa_nodes[cpu_id], &nodemask_bm);
    }

    return g->init_err;
}

static int memkind_low_lat_loc_preferred_get_mbind_nodemask(
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct loc_numanodes_t *g =
        &memkind_low_lat_loc_numanodes_g[NODE_VARIANT_SINGLE];

    pthread_once(&memkind_low_lat_loc_numanodes_once_g[NODE_VARIANT_SINGLE],
                 memkind_low_lat_loc_preferred_numanodes_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        int cpu_id = sched_getcpu();
        struct bitmask nodemask_bm = {maxnode, nodemask};
        copy_bitmask_to_bitmask(g->per_cpu_numa_nodes[cpu_id], &nodemask_bm);
    }

    return g->init_err;
}

static int memkind_hi_bw_loc_get_mbind_nodemask(struct memkind *kind,
                                                unsigned long *nodemask,
                                                unsigned long maxnode)
{
    struct loc_numanodes_t *g =
        &memkind_hi_bw_loc_numanodes_g[NODE_VARIANT_MULTIPLE];
    pthread_once(&memkind_hi_bw_loc_numanodes_once_g[NODE_VARIANT_MULTIPLE],
                 memkind_hi_bw_loc_numanodes_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        int cpu_id = sched_getcpu();
        struct bitmask nodemask_bm = {maxnode, nodemask};
        copy_bitmask_to_bitmask(g->per_cpu_numa_nodes[cpu_id], &nodemask_bm);
    }

    return g->init_err;
}

static int memkind_hi_bw_loc_preferred_get_mbind_nodemask(
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct loc_numanodes_t *g =
        &memkind_hi_bw_loc_numanodes_g[NODE_VARIANT_SINGLE];

    pthread_once(&memkind_hi_bw_loc_numanodes_once_g[NODE_VARIANT_SINGLE],
                 memkind_hi_bw_loc_preferred_numanodes_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        int cpu_id = sched_getcpu();
        struct bitmask nodemask_bm = {maxnode, nodemask};
        copy_bitmask_to_bitmask(g->per_cpu_numa_nodes[cpu_id], &nodemask_bm);
    }

    return g->init_err;
}

static int memkind_hi_cap_loc_finalize(memkind_t kind)
{
    local_kind_finalize(&memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_MULTIPLE]);
    return memkind_arena_finalize(kind);
}

static int memkind_hi_cap_loc_preferred_finalize(memkind_t kind)
{
    local_kind_finalize(&memkind_hi_cap_loc_numanodes_g[NODE_VARIANT_SINGLE]);
    return memkind_arena_finalize(kind);
}

static int memkind_low_lat_loc_finalize(memkind_t kind)
{
    local_kind_finalize(
        &memkind_low_lat_loc_numanodes_g[NODE_VARIANT_MULTIPLE]);
    return memkind_arena_finalize(kind);
}

static int memkind_low_lat_loc_preferred_finalize(memkind_t kind)
{
    local_kind_finalize(&memkind_low_lat_loc_numanodes_g[NODE_VARIANT_SINGLE]);
    return memkind_arena_finalize(kind);
}

static int memkind_hi_bw_loc_finalize(memkind_t kind)
{
    local_kind_finalize(&memkind_hi_bw_loc_numanodes_g[NODE_VARIANT_MULTIPLE]);
    return memkind_arena_finalize(kind);
}

static int memkind_hi_bw_loc_preferred_finalize(memkind_t kind)
{
    local_kind_finalize(&memkind_hi_bw_loc_numanodes_g[NODE_VARIANT_SINGLE]);
    return memkind_arena_finalize(kind);
}

static int memkind_loc_check_available(struct memkind *kind)
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

static void memkind_low_lat_loc_init_once(void)
{
    memkind_init(MEMKIND_LOWEST_LATENCY_LOCAL, true);
}

static void memkind_low_lat_loc_preferred_init_once(void)
{
    memkind_init(MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED, true);
}
static void memkind_hi_bw_loc_init_once(void)
{
    memkind_init(MEMKIND_HIGHEST_BANDWIDTH_LOCAL, true);
}

static void memkind_hi_bw_loc_preferred_init_once(void)
{
    memkind_init(MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED, true);
}

MEMKIND_EXPORT struct memkind_ops MEMKIND_HIGHEST_CAPACITY_LOCAL_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_loc_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hi_cap_loc_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hi_cap_loc_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_hi_cap_loc_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED_OPS =
    {.create = memkind_arena_create,
     .destroy = memkind_default_destroy,
     .malloc = memkind_arena_malloc,
     .calloc = memkind_arena_calloc,
     .posix_memalign = memkind_arena_posix_memalign,
     .realloc = memkind_arena_realloc,
     .free = memkind_arena_free,
     .check_available = memkind_loc_check_available,
     .mbind = memkind_default_mbind,
     .get_mmap_flags = memkind_default_get_mmap_flags,
     .get_mbind_mode = memkind_preferred_get_mbind_mode,
     .get_mbind_nodemask = memkind_hi_cap_loc_preferred_get_mbind_nodemask,
     .get_arena = memkind_thread_get_arena,
     .init_once = memkind_hi_cap_loc_preferred_init_once,
     .malloc_usable_size = memkind_default_malloc_usable_size,
     .finalize = memkind_hi_cap_loc_preferred_finalize,
     .get_stat = memkind_arena_get_kind_stat,
     .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_LOWEST_LATENCY_LOCAL_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_loc_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_low_lat_loc_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_low_lat_loc_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_low_lat_loc_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_loc_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_low_lat_loc_preferred_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_low_lat_loc_preferred_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_low_lat_loc_preferred_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HIGHEST_BANDWIDTH_LOCAL_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_loc_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hi_bw_loc_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hi_bw_loc_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_hi_bw_loc_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops
    MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED_OPS = {
        .create = memkind_arena_create,
        .destroy = memkind_default_destroy,
        .malloc = memkind_arena_malloc,
        .calloc = memkind_arena_calloc,
        .posix_memalign = memkind_arena_posix_memalign,
        .realloc = memkind_arena_realloc,
        .free = memkind_arena_free,
        .check_available = memkind_loc_check_available,
        .mbind = memkind_default_mbind,
        .get_mmap_flags = memkind_default_get_mmap_flags,
        .get_mbind_mode = memkind_preferred_get_mbind_mode,
        .get_mbind_nodemask = memkind_hi_bw_loc_preferred_get_mbind_nodemask,
        .get_arena = memkind_thread_get_arena,
        .init_once = memkind_hi_bw_loc_preferred_init_once,
        .malloc_usable_size = memkind_default_malloc_usable_size,
        .finalize = memkind_hi_bw_loc_preferred_finalize,
        .get_stat = memkind_arena_get_kind_stat,
        .defrag_reallocate = memkind_arena_defrag_reallocate};
