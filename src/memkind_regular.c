// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2020 Intel Corporation. */

#include <memkind.h>
#include <memkind/internal/heap_manager.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_default.h>

#include <numa.h>

static struct bitmask *regular_nodes_mask = NULL;

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

static void memkind_regular_init_once(void)
{
    regular_nodes_init();
    memkind_init(MEMKIND_REGULAR, true);
}

static int memkind_regular_check_available(struct memkind *kind)
{
    /* init_once method is called in memkind_malloc function
     * when memkind malloc is not called this function will fail.
     * Call pthread_once to be sure that situation mentioned
     * above will never happen */
    pthread_once(&kind->init_once, kind->ops->init_once);
    return regular_nodes_mask != NULL ? MEMKIND_SUCCESS
                                      : MEMKIND_ERROR_UNAVAILABLE;
}

MEMKIND_EXPORT int memkind_regular_all_get_mbind_nodemask(
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};

    if (!regular_nodes_mask) {
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    copy_bitmask_to_bitmask(regular_nodes_mask, &nodemask_bm);
    return MEMKIND_SUCCESS;
}

static int memkind_regular_finalize(memkind_t kind)
{
    if (regular_nodes_mask)
        numa_bitmask_free(regular_nodes_mask);

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
    .defrag_reallocate = memkind_arena_defrag_reallocate};
