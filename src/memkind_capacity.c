// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/memkind_log.h>

#include "config.h"

#include <numa.h>
#ifdef MEMKIND_HWLOC
#include <hwloc.h>
#endif

struct memkind_loc_hi_cap_numanodes_t {
    int init_err;
    struct bitmask **per_cpu_numa_nodes;
};

struct hi_cap_numanodes_t {
    int init_err;
    struct bitmask *numa_nodes;
};

#define NODE_VARIANT_MULTIPLE 0
#define NODE_VARIANT_SINGLE   1
#define NODE_VARIANT_MAX      2

static struct memkind_loc_hi_cap_numanodes_t memkind_loc_hi_cap_numanodes_g;
static pthread_once_t memkind_loc_hi_cap_numanodes_once_g = PTHREAD_ONCE_INIT;

static struct hi_cap_numanodes_t memkind_hi_cap_numanodes_g[NODE_VARIANT_MAX];
static pthread_once_t memkind_hi_cap_numanodes_once_g[NODE_VARIANT_MAX] = {PTHREAD_ONCE_INIT};

#ifdef MEMKIND_HWLOC
static bool has_same_parrent(hwloc_obj_t initiatior_object,
                             hwloc_obj_t numa_object)
{
    hwloc_obj_t initiator_parent = initiatior_object;
    while (initiator_parent->parent) {
        initiator_parent = initiator_parent->parent;
        if ((initiator_parent->type == HWLOC_OBJ_GROUP) ||
            (initiator_parent->type == HWLOC_OBJ_PACKAGE)) {
            break;
        }
    }

    hwloc_obj_t numa_parent = numa_object;
    while (numa_parent->parent) {
        numa_parent = numa_parent->parent;
        if ((initiator_parent->type == HWLOC_OBJ_GROUP) ||
            (initiator_parent->type == HWLOC_OBJ_PACKAGE)) {
            break;
        }
    }

    return (initiator_parent && numa_parent && (initiator_parent == numa_parent));
}

static void memkind_loc_hi_cap_numanodes_init(void)
{
    int num_cpus = numa_num_configured_cpus();
    int max_node_id = numa_max_node();
    hwloc_topology_t topology;
    struct memkind_loc_hi_cap_numanodes_t *g = &memkind_loc_hi_cap_numanodes_g;
    int i;

    int err = hwloc_topology_init(&topology);
    if (MEMKIND_UNLIKELY(err)) {
        log_err("hwloc initialize failed");
        g->init_err = MEMKIND_ERROR_UNAVAILABLE;
        return;
    }

    err = hwloc_topology_load(topology);
    if (MEMKIND_UNLIKELY(err)) {
        log_err("hwloc topology load failed");
        hwloc_topology_destroy(topology);
        g->init_err = MEMKIND_ERROR_UNAVAILABLE;
        return;
    }

    // Allocate a bitmask for HIGHEST_CAPACITY_LOCAL NUMA nodes on the system.
    g->per_cpu_numa_nodes = calloc(sizeof(struct bitmask *), num_cpus);
    if (MEMKIND_UNLIKELY(g->per_cpu_numa_nodes == NULL)) {
        g->init_err = MEMKIND_ERROR_MALLOC;
        log_err("malloc() failed.");
        goto error;
    }

    // Iterate over all Processing Unit objects
    hwloc_obj_t cpu_object = NULL;
    while ((cpu_object = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_PU,
                                                    cpu_object)) != NULL) {

        // allocate a NUMA bitmask for HIGHEST_LOCAL_CAPACITY nodes if it is not
        // allocated yet
        struct bitmask *numa_bitmask = g->per_cpu_numa_nodes[cpu_object->os_index];
        if (numa_bitmask == NULL) {
            numa_bitmask = numa_bitmask_alloc(max_node_id + 1);
            if (MEMKIND_UNLIKELY(numa_bitmask == NULL)) {
                g->init_err = MEMKIND_ERROR_MALLOC;
                log_err("malloc() failed.");
                goto error;
            }
            g->per_cpu_numa_nodes[cpu_object->os_index] = numa_bitmask;
        }

        // iterate over all NUMA nodes that has the same parent (Group or
        // Package object) that innitiator node and add all HIGHEST_CAPACITY_LOCAL
        // nodes to bitmask
        long long best_capacity = 0;
        hwloc_obj_t numa_object = NULL;
        while ((numa_object = hwloc_get_next_obj_by_type(topology,
                                                         HWLOC_OBJ_NUMANODE, numa_object)) != NULL) {

            if (has_same_parrent(cpu_object, numa_object) == false) {
                continue;
            }

            long long current_capacity = numa_node_size64(numa_object->os_index, NULL);
            if (current_capacity == -1) {
                continue;
            }

            if (current_capacity > best_capacity) {
                best_capacity = current_capacity;
                numa_bitmask_clearall(numa_bitmask);
            }

            if (current_capacity == best_capacity) {
                numa_bitmask_setbit(numa_bitmask, numa_object->os_index);
            }
        }
    }

    g->init_err = MEMKIND_SUCCESS;
    goto success;

error:
    for(i = 0; i <= max_node_id; ++i) {
        if (g->per_cpu_numa_nodes[i]) {
            numa_bitmask_free(g->per_cpu_numa_nodes[i]);
        }
    }
    free(g->per_cpu_numa_nodes);

success:
    hwloc_topology_destroy(topology);

    return;
}
#else
static void memkind_loc_hi_cap_numanodes_init(void)
{
    log_err("Can't automatically detect NUMA nodes locality info.");
    g->init_err = MEMKIND_ERROR_OPERATION_FAILED;
}
#endif // MEMKIND_HWLOC

static int memkind_loc_hi_cap_get_mbind_nodemask(
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct memkind_loc_hi_cap_numanodes_t *g = &memkind_loc_hi_cap_numanodes_g;

    pthread_once(&memkind_loc_hi_cap_numanodes_once_g,
                 memkind_loc_hi_cap_numanodes_init);

    int cpu_id = sched_getcpu();

    if (MEMKIND_LIKELY(!g->init_err &&
                       g->per_cpu_numa_nodes &&
                       g->per_cpu_numa_nodes[cpu_id])) {
        struct bitmask nodemask_bm = {maxnode, nodemask};
        copy_bitmask_to_bitmask(g->per_cpu_numa_nodes[cpu_id], &nodemask_bm);
    }

    return g->init_err;
}

static int memkind_loc_hi_cap_finalize(memkind_t kind)
{
    struct memkind_loc_hi_cap_numanodes_t *g = &memkind_loc_hi_cap_numanodes_g;

    int max_node_id = numa_max_node();
    int node;

    if (MEMKIND_LIKELY(!g->init_err && g->per_cpu_numa_nodes)) {
        for (node = 0; node <= max_node_id; ++node) {
            if (g->per_cpu_numa_nodes[node]) {
                numa_bitmask_free(g->per_cpu_numa_nodes[node]);
            }
        }
    }

    free(g->per_cpu_numa_nodes);
    return 0;
}

static int memkind_loc_hi_cap_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

static void memkind_loc_hi_cap_init_once(void)
{
    memkind_init(MEMKIND_HIGHEST_CAPACITY, true);
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
    .check_available = memkind_loc_hi_cap_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_loc_hi_cap_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_loc_hi_cap_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_loc_hi_cap_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate
};
