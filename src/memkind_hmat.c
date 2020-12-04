// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

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
#include <sched.h>

#if HAVE_LIBHWLOC
#include <hwloc.h>
#endif // HAVE_LIBHWLOC

struct hi_cap_numanodes_t { 
    int init_err; 
    struct bitmask* numa_nodes; 
}; 

static struct hi_cap_numanodes_t memkind_hi_cap_numanodes_g; 
static pthread_once_t memkind_hi_cap_numanodes_once_g = PTHREAD_ONCE_INIT; 

#if HAVE_LIBHWLOC
// TODO add hwloc implementation
#else
static inline long long get_hi_cap_current(int node)
{
    // for highest capacity, get node capacity using libnuma
    return numa_node_size64(node, NULL);
}
#endif

static void memkind_hi_cap_numanodes_init(void) 
{ 
    long long best = 0;  
    /* TODO node or CPU? I saw initializing this to numa_num_configured_cpus() for HBW */ 
    int max_node = numa_num_configured_nodes();

    struct hi_cap_numanodes_t *g = &memkind_hi_cap_numanodes_g; 
    g->numa_nodes = numa_bitmask_alloc(max_node); /* TODO where to free this? */ 

    if (MEMKIND_UNLIKELY(g->numa_nodes == NULL)) { 
        g->init_err = MEMKIND_ERROR_MALLOC; 
        log_err("malloc() failed."); 
        return; 
    } 

    numa_bitmask_clearall(g->numa_nodes); 

    for (int i = 0; i < max_node; i++) { 
        long long current = get_hi_cap_current(i); 

        if (current > best) { 
            /* found new best, set it and clear results */ 
            best = current; 
            numa_bitmask_clearall(g->numa_nodes); 
        } 

        if (current == best) { 
            /* add result to bitmask */ 
            numa_bitmask_setbit(g->numa_nodes, i); 
        } 
    } 

    if (numa_bitmask_weight(g->numa_nodes) == 0) {
        // there should be always at least one NUMA node found
        g->init_err = MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
    }

    g->init_err = MEMKIND_SUCCESS; 
} 

static int memkind_hi_cap_get_mbind_nodemask( 
    struct memkind *kind, unsigned long *nodemask, unsigned long maxnode) 
{ 
    struct hi_cap_numanodes_t *g = &memkind_hi_cap_numanodes_g; 
    pthread_once(&memkind_hi_cap_numanodes_once_g, 
                 memkind_hi_cap_numanodes_init); 
    
    if (MEMKIND_LIKELY(!g->init_err)) { 
        struct bitmask nodemask_bm = {maxnode, nodemask}; 
        numa_bitmask_clearall(&nodemask_bm); 
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
    .finalize = memkind_arena_finalize, 
    .get_stat = memkind_arena_get_kind_stat, 
    .defrag_reallocate = memkind_arena_defrag_reallocate 
};
