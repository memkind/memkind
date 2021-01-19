// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2021 Intel Corporation. */

#include <memkind.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_cpu.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/heap_manager.h>

#include <numa.h>

static struct bitmask *regular_nodes_mask = NULL;

struct cpu_local_numanodes_t {
    int init_err;
    int *numa_nodes;
};

static struct cpu_local_numanodes_t memkind_cpu_local_numanodes_g;
static pthread_once_t memkind_cpu_local_numanodes_once_g = PTHREAD_ONCE_INIT;

static void regular_nodes_init(void)
{
    unsigned i;
    unsigned nodes_num = (unsigned)numa_num_configured_nodes();
    struct bitmask *node_cpus = numa_allocate_cpumask();

    regular_nodes_mask = numa_allocate_nodemask();

    for (i = 0; i < nodes_num; i++) {
        numa_node_to_cpus(i, node_cpus);
        if (numa_bitmask_weight(node_cpus))
            numa_bitmask_setbit(regular_nodes_mask, i);
    }
    numa_bitmask_free(node_cpus);
}

static void cpu_local_nodes_init(void)
{
    unsigned cpus_no = numa_num_configured_cpus();
    struct cpu_local_numanodes_t *g = &memkind_cpu_local_numanodes_g;
    int max_node_id = numa_max_node();
    struct bitmask *node_cpu_mask = numa_allocate_cpumask();
    if (!node_cpu_mask) {
        log_err("numa_allocate_cpumask() failed.");
        g->init_err = MEMKIND_ERROR_MALLOC;
        return;
    }
    unsigned node_id, cpu_id;
    g->numa_nodes = malloc(sizeof(int)* cpus_no);
    if (!g->numa_nodes) {
        free(node_cpu_mask);
        log_err("malloc() failed.");
        g->init_err = MEMKIND_ERROR_MALLOC;
        return;
    }

    for (node_id = 0; node_id <= max_node_id; ++node_id) {
        if (numa_node_to_cpus(node_id, node_cpu_mask) != 0)
            continue;
        if (numa_bitmask_weight(node_cpu_mask) && numa_node_size64(node_id,NULL) > 0) {
            for (cpu_id = 0; cpu_id < cpus_no; ++cpu_id) {
                if(numa_bitmask_isbitset(node_cpu_mask, cpu_id))
                    g->numa_nodes[cpu_id] = node_id;
            }
        }
    }

    free(node_cpu_mask);
    g->init_err = MEMKIND_SUCCESS;
}

static void memkind_regular_init_once(void)
{
    regular_nodes_init();
    memkind_init(MEMKIND_REGULAR, true);
}

static void memkind_cpu_local_init_once(void)
{
    memkind_init(MEMKIND_CPU_LOCAL, true);
}

static int memkind_regular_check_available(struct memkind *kind)
{
    /* init_once method is called in memkind_malloc function
     * when memkind malloc is not called this function will fail.
     * Call pthread_once to be sure that situation mentioned
     * above will never happen */
    pthread_once(&kind->init_once, kind->ops->init_once);
    return regular_nodes_mask != NULL ? MEMKIND_SUCCESS : MEMKIND_ERROR_UNAVAILABLE;
}

static int memkind_cpu_local_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

MEMKIND_EXPORT int memkind_regular_all_get_mbind_nodemask(struct memkind *kind,
                                                          unsigned long *nodemask,
                                                          unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};

    if(!regular_nodes_mask) {
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    copy_bitmask_to_bitmask(regular_nodes_mask, &nodemask_bm);
    return MEMKIND_SUCCESS;
}

static int memkind_cpu_local_get_mbind_nodemask(struct memkind *kind,
                                                unsigned long *nodemask,
                                                unsigned long maxnode)
{
    struct cpu_local_numanodes_t *g = &memkind_cpu_local_numanodes_g;
    pthread_once(&memkind_cpu_local_numanodes_once_g, cpu_local_nodes_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        int cpu = sched_getcpu();
        struct bitmask nodemask_bm = {maxnode, nodemask};
        numa_bitmask_clearall(&nodemask_bm);
        numa_bitmask_setbit(&nodemask_bm, g->numa_nodes[cpu]);
    }

    return g->init_err;
}

static int memkind_regular_finalize(memkind_t kind)
{
    if(regular_nodes_mask)
        numa_bitmask_free(regular_nodes_mask);

    return memkind_arena_finalize(kind);
}

static int memkind_cpu_local_finalize(memkind_t kind)
{
    struct cpu_local_numanodes_t *g = &memkind_cpu_local_numanodes_g;
    if(g->numa_nodes)
        free(g->numa_nodes);

    return memkind_arena_finalize(kind);
}

MEMKIND_EXPORT struct memkind_ops MEMKIND_REGULAR_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_regular_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_regular_all_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_regular_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_regular_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

MEMKIND_EXPORT struct memkind_ops MEMKIND_CPU_LOCAL_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_cpu_local_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_cpu_local_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_cpu_local_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_cpu_local_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate
};
