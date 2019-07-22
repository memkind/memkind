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

#include <memkind/internal/memkind_dax_kmem.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/heap_manager.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <jemalloc/jemalloc.h>
#include <utmpx.h>
#include <sched.h>
#include <stdint.h>
#include <assert.h>

MEMKIND_EXPORT struct memkind_ops MEMKIND_DAX_KMEM_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_dax_kmem_check_available, // TODO
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_dax_kmem_get_mbind_nodemask, // TODO
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_dax_kmem_init_once, // TODO
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize
};

struct numanode_bandwidth_t {
    int numanode;
    int bandwidth;
};

struct bandwidth_nodes_t {
    int bandwidth;
    int num_numanodes;
    int *numanodes;
};

struct memkind_dax_kmem_closest_numanode_t {
    int init_err;
    int num_cpu;
    int *closest_numanode;
};

static struct memkind_dax_kmem_closest_numanode_t
    memkind_dax_kmem_closest_numanode_g;
static pthread_once_t memkind_dax_kmem_closest_numanode_once_g =
    PTHREAD_ONCE_INIT;

static void memkind_dax_kmem_closest_numanode_init(void);

static int create_bandwidth_nodes(int num_bandwidth, const int *bandwidth,
                                  int *num_unique, struct bandwidth_nodes_t **bandwidth_nodes);

static int set_closest_numanode(int num_unique,
                                const struct bandwidth_nodes_t *bandwidth_nodes,
                                int target_bandwidth, int num_cpunode, int *closest_numanode);

static int numanode_bandwidth_compare(const void *a, const void *b);

// This declaration is necesarry, cause it's missing in headers from libnuma 2.0.8
extern unsigned int numa_bitmask_weight(const struct bitmask *bmp );

static void assign_arbitrary_bandwidth_values(int *bandwidth, int bandwidth_len,
                                              struct bitmask *dax_kmem_nodes);

static int fill_bandwidth_values_from_enviroment(int *bandwidth,
                                                 int bandwidth_len, char *dax_kmem_nodes_env);

static int fill_nodes_bandwidth(int *bandwidth, int bandwidth_len);

MEMKIND_EXPORT int memkind_dax_kmem_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

MEMKIND_EXPORT int memkind_dax_kmem_get_mbind_nodemask(struct memkind *kind,
                                                       unsigned long *nodemask,
                                                       unsigned long maxnode)
{
    int cpu;
    struct bitmask nodemask_bm = {maxnode, nodemask};
    struct memkind_dax_kmem_closest_numanode_t *g =
            &memkind_dax_kmem_closest_numanode_g;
    pthread_once(&memkind_dax_kmem_closest_numanode_once_g,
                 memkind_dax_kmem_closest_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err && nodemask)) {
        numa_bitmask_clearall(&nodemask_bm);
        cpu = sched_getcpu();
        if (MEMKIND_LIKELY(cpu < g->num_cpu)) {
            numa_bitmask_setbit(&nodemask_bm, g->closest_numanode[cpu]);
        } else {
            return MEMKIND_ERROR_RUNTIME;
        }
    }
    return g->init_err;
}

MEMKIND_EXPORT int memkind_dax_kmem_all_get_mbind_nodemask(
    struct memkind *kind,
    unsigned long *nodemask,
    unsigned long maxnode)
{
    int cpu;
    struct bitmask nodemask_bm = {maxnode, nodemask};
    struct memkind_dax_kmem_closest_numanode_t *g =
            &memkind_dax_kmem_closest_numanode_g;
    pthread_once(&memkind_dax_kmem_closest_numanode_once_g,
                 memkind_dax_kmem_closest_numanode_init);

    if (MEMKIND_LIKELY(!g->init_err && nodemask)) {
        numa_bitmask_clearall(&nodemask_bm);
        for (cpu = 0; cpu < g->num_cpu; ++cpu) {
            numa_bitmask_setbit(&nodemask_bm, g->closest_numanode[cpu]);
        }
    }
    return g->init_err;
}

static void assign_arbitrary_bandwidth_values(int *bandwidth, int bandwidth_len,
                                              struct bitmask *hbw_nodes)
{
    int i, nodes_num = numa_num_configured_nodes();

    // Assigning arbitrary bandwidth values for nodes:
    // 2 - high BW node (if bit set in hbw_nodes nodemask),
    // 1 - low  BW node,
    // 0 - node not present
    for (i=0; i<NUMA_NUM_NODES; i++) {
        if (i >= nodes_num) {
            bandwidth[i] = 0;
        } else if (numa_bitmask_isbitset(hbw_nodes, i)) {
            bandwidth[i] = 2;
        } else {
            bandwidth[i] = 1;
        }
    }

}

static int fill_bandwidth_values_from_enviroment(int *bandwidth,
                                                 int bandwidth_len, char *kmem_nodes_env)
{
    struct bitmask *kmem_nodes_bm = numa_parse_nodestring(kmem_nodes_env);

    if (!kmem_nodes_bm) {
        log_err("Wrong MEMKIND_DAX_KMEM environment value.");
        return MEMKIND_ERROR_ENVIRON;
    } else {
        assign_arbitrary_bandwidth_values(bandwidth, bandwidth_len, kmem_nodes_bm);
        numa_bitmask_free(kmem_nodes_bm);
    }

    return 0;
}

static int fill_nodes_bandwidth(int *bandwidth, int bandwidth_len)
{
    char *dax_kmem_nodes_env;

    dax_kmem_nodes_env = getenv("MEMKIND_DAX_KMEM_NODES");
    if (dax_kmem_nodes_env) {
        log_info("Environment variable MEMKIND_DAX_KMEM_NODES detected: %s.",
                 dax_kmem_nodes_env);
        return fill_bandwidth_values_from_enviroment(bandwidth, bandwidth_len,
                                                     dax_kmem_nodes_env);
    }

    /* ASSERT HERE */
    return -1;
}

static void memkind_dax_kmem_closest_numanode_init(void) // TODO(KFilipek)
{
    struct memkind_dax_kmem_closest_numanode_t *g =
            &memkind_dax_kmem_closest_numanode_g;
    int *bandwidth = NULL;
    int num_unique = 0;
    int high_bandwidth = 0;
    int i;

    struct bandwidth_nodes_t *bandwidth_nodes = NULL;

    g->num_cpu = numa_num_configured_cpus();
    g->closest_numanode = (int *)jemk_malloc(sizeof(int) * g->num_cpu);
    bandwidth = (int *)jemk_malloc(sizeof(int) * NUMA_NUM_NODES);

    if (!(g->closest_numanode && bandwidth)) {
        g->init_err = MEMKIND_ERROR_MALLOC;
        log_err("jemk_malloc() failed.");
        goto exit;
    }

    g->init_err = fill_nodes_bandwidth(bandwidth, NUMA_NUM_NODES);
    if (g->init_err)
        goto exit;

    g->init_err = create_bandwidth_nodes(NUMA_NUM_NODES, bandwidth,
                                         &num_unique, &bandwidth_nodes);
    if (g->init_err)
        goto exit;

    high_bandwidth = bandwidth_nodes[num_unique-1].bandwidth;
    g->init_err = set_closest_numanode(num_unique, bandwidth_nodes,
                                       high_bandwidth, g->num_cpu,
                                       g->closest_numanode);

    for(i=0; i<bandwidth_nodes[num_unique-1].num_numanodes; i++) {
        log_info("NUMA node %d is DAX KMEM memory.",
                 bandwidth_nodes[num_unique-1].numanodes[i]);
    }

exit:

    jemk_free(bandwidth_nodes);
    jemk_free(bandwidth);

    if (g->init_err) {
        jemk_free(g->closest_numanode);
        g->closest_numanode = NULL;
    }
}


static int create_bandwidth_nodes(int num_bandwidth, const int *bandwidth,
                                  int *num_unique, struct bandwidth_nodes_t **bandwidth_nodes)
{
    /***************************************************************************
    *   num_bandwidth (IN):                                                    *
    *       number of numa nodes and length of bandwidth vector.               *
    *   bandwidth (IN):                                                        *
    *       A vector of length num_bandwidth that gives bandwidth for          *
    *       each numa node, zero if numa node has unknown bandwidth.           *
    *   num_unique (OUT):                                                      *
    *       number of unique non-zero bandwidth values in bandwidth            *
    *       vector.                                                            *
    *   bandwidth_nodes (OUT):                                                 *
    *       A list of length num_unique sorted by bandwidth value where        *
    *       each element gives a list of the numa nodes that have the          *
    *       given bandwidth.                                                   *
    *   RETURNS zero on success, error code on failure                         *
    ***************************************************************************/
    int err = 0;
    int i, j, k, l, last_bandwidth;
    struct numanode_bandwidth_t *numanode_bandwidth = NULL;
    *bandwidth_nodes = NULL;
    /* allocate space for sorting array */
    numanode_bandwidth = jemk_malloc(sizeof(struct numanode_bandwidth_t) *
                                     num_bandwidth);
    if (!numanode_bandwidth) {
        err = MEMKIND_ERROR_MALLOC;
        log_err("jemk_malloc() failed.");
    }
    if (!err) {
        /* set sorting array */
        j = 0;
        for (i = 0; i < num_bandwidth; ++i) {
            if (bandwidth[i] != 0) {
                numanode_bandwidth[j].numanode = i;
                numanode_bandwidth[j].bandwidth = bandwidth[i];
                ++j;
            }
        }
        /* ignore zero bandwidths */
        num_bandwidth = j;
        if (num_bandwidth == 0) {
            err = MEMKIND_ERROR_UNAVAILABLE;
        }
    }
    if (!err) {
        qsort(numanode_bandwidth, num_bandwidth,
              sizeof(struct numanode_bandwidth_t), numanode_bandwidth_compare);
        /* calculate the number of unique bandwidths */
        *num_unique = 1;
        last_bandwidth = numanode_bandwidth[0].bandwidth;
        for (i = 1; i < num_bandwidth; ++i) {
            if (numanode_bandwidth[i].bandwidth != last_bandwidth) {
                last_bandwidth = numanode_bandwidth[i].bandwidth;
                ++*num_unique;
            }
        }
        /* allocate output array */
        *bandwidth_nodes = (struct bandwidth_nodes_t *)jemk_malloc(
                               sizeof(struct bandwidth_nodes_t) * (*num_unique) +
                               sizeof(int) * num_bandwidth);
        if (!*bandwidth_nodes) {
            err = MEMKIND_ERROR_MALLOC;
            log_err("jemk_malloc() failed.");
        }
    }
    if (!err) {
        /* populate output */
        (*bandwidth_nodes)[0].numanodes = (int *)(*bandwidth_nodes + *num_unique);
        last_bandwidth = numanode_bandwidth[0].bandwidth;
        k = 0;
        l = 0;
        for (i = 0; i < num_bandwidth; ++i, ++l) {
            (*bandwidth_nodes)[0].numanodes[i] = numanode_bandwidth[i].numanode;
            if (numanode_bandwidth[i].bandwidth != last_bandwidth) {
                (*bandwidth_nodes)[k].num_numanodes = l;
                (*bandwidth_nodes)[k].bandwidth = last_bandwidth;
                l = 0;
                ++k;
                (*bandwidth_nodes)[k].numanodes = (*bandwidth_nodes)[0].numanodes + i;
                last_bandwidth = numanode_bandwidth[i].bandwidth;
            }
        }
        (*bandwidth_nodes)[k].num_numanodes = l;
        (*bandwidth_nodes)[k].bandwidth = last_bandwidth;
    }
    if (numanode_bandwidth) {
        jemk_free(numanode_bandwidth);
    }
    if (err) {
        if (*bandwidth_nodes) {
            jemk_free(*bandwidth_nodes);
        }
    }
    return err;
}

static int set_closest_numanode(int num_unique,
                                const struct bandwidth_nodes_t *bandwidth_nodes,
                                int target_bandwidth, int num_cpunode, int *closest_numanode)
{
    /***************************************************************************
    *   num_unique (IN):                                                       *
    *       Length of bandwidth_nodes vector.                                  *
    *   bandwidth_nodes (IN):                                                  *
    *       Output vector from create_bandwitdth_nodes().                      *
    *   target_bandwidth (IN):                                                 *
    *       The bandwidth to select for comparison.                            *
    *   num_cpunode (IN):                                                      *
    *       Number of cpu's and length of closest_numanode.                    *
    *   closest_numanode (OUT):                                                *
    *       Vector that maps cpu index to closest numa node of the specified   *
    *       bandwidth.                                                         *
    *   RETURNS zero on success, error code on failure                         *
    ***************************************************************************/
    int err = 0;
    int min_distance, distance, i, j, old_errno, min_unique;
    struct bandwidth_nodes_t match;
    match.bandwidth = -1;
    for (i = 0; i < num_cpunode; ++i) {
        closest_numanode[i] = -1;
    }
    for (i = 0; i < num_unique; ++i) {
        if (bandwidth_nodes[i].bandwidth == target_bandwidth) {
            match = bandwidth_nodes[i];
            break;
        }
    }
    if (match.bandwidth == -1) {
        err = MEMKIND_ERROR_UNAVAILABLE;
    } else {
        for (i = 0; i < num_cpunode; ++i) {
            min_distance = INT_MAX;
            min_unique = 1;
            for (j = 0; j < match.num_numanodes; ++j) {
                old_errno = errno;
                distance = numa_distance(numa_node_of_cpu(i),
                                         match.numanodes[j]);
                errno = old_errno;
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_numanode[i] = match.numanodes[j];
                    min_unique = 1;
                } else if (distance == min_distance) {
                    min_unique = 0;
                }
            }
            if (!min_unique) {
                err = MEMKIND_ERROR_RUNTIME;
            }
        }
    }
    return err;
}

static int numanode_bandwidth_compare(const void *a, const void *b)
{
    /***************************************************************************
    *  qsort comparison function for numa_node_bandwidth structures.  Sorts in *
    *  order of bandwidth and then numanode.                                   *
    ***************************************************************************/
    int result;
    struct numanode_bandwidth_t *aa = (struct numanode_bandwidth_t *)(a);
    struct numanode_bandwidth_t *bb = (struct numanode_bandwidth_t *)(b);
    result = (aa->bandwidth > bb->bandwidth) - (aa->bandwidth < bb->bandwidth);
    if (result == 0) {
        result = (aa->numanode > bb->numanode) - (aa->numanode < bb->numanode);
    }
    return result;
}

MEMKIND_EXPORT void memkind_dax_kmem_init_once(void)
{
    memkind_init(MEMKIND_DAX_KMEM, true);
}
