/*
 * Copyright (C) 2014, 2015 Intel Corporation.
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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <jemalloc/jemalloc.h>
#include <utmpx.h>
#include <sched.h>
#include <stdint.h>


#include <memkind/internal/memkind_hbw.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_hugetlb.h>
#include <memkind/internal/memkind_arena.h>

const struct memkind_ops MEMKIND_HBW_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .check_available = memkind_hbw_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .get_size = memkind_default_get_size,
    .init_once = memkind_hbw_init_once,
};

const struct memkind_ops MEMKIND_HBW_HUGETLB_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .check_available = memkind_hbw_hugetlb_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .get_size = memkind_default_get_size,
    .init_once = memkind_hbw_hugetlb_init_once,
};

const struct memkind_ops MEMKIND_HBW_PREFERRED_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .check_available = memkind_hbw_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .get_size = memkind_default_get_size,
    .init_once = memkind_hbw_preferred_init_once,
};

const struct memkind_ops MEMKIND_HBW_PREFERRED_HUGETLB_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .check_available = memkind_hbw_hugetlb_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_mbind_mode = memkind_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .get_size = memkind_default_get_size,
    .init_once = memkind_hbw_preferred_hugetlb_init_once,
};

const struct memkind_ops MEMKIND_HBW_INTERLEAVE_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_arena_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .check_available = memkind_hbw_check_available,
    .mbind = memkind_default_mbind,
    .madvise = memkind_nohugepage_madvise,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_interleave_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_all_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .get_size = memkind_default_get_size,
    .init_once = memkind_hbw_interleave_init_once,
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

struct memkind_hbw_closest_numanode_t {
    int init_err;
    int num_cpu;
    int *closest_numanode;
};

static struct memkind_hbw_closest_numanode_t memkind_hbw_closest_numanode_g;
static pthread_once_t memkind_hbw_closest_numanode_once_g = PTHREAD_ONCE_INIT;

static void memkind_hbw_closest_numanode_init(void);

static int parse_node_bandwidth(int num_bandwidth, int *bandwidth,
                                const char *bandwidth_path);

static int create_bandwidth_nodes(int num_bandwidth, const int *bandwidth,
                                  int *num_unique, struct bandwidth_nodes_t **bandwidth_nodes);

static int set_closest_numanode(int num_unique,
                                const struct bandwidth_nodes_t *bandwidth_nodes,
                                int target_bandwidth, int num_cpunode, int *closest_numanode);

static int numanode_bandwidth_compare(const void *a, const void *b);

// This declaration is necesarry, cause it's missing in headers from libnuma 2.0.8
extern unsigned int numa_bitmask_weight(const struct bitmask *bmp );

static void assign_arbitrary_bandwidth_values(int* bandwidth, int bandwidth_len, struct bitmask* hbw_nodes);

inline static void cpuid_asm(int leaf, int subleaf, uint32_t where[4]);

static int is_cpu_xeon_phi_x200();

static int fill_bandwidth_values_heuristically (int* bandwidth, int bandwidth_len);

static int fill_bandwidth_values_from_enviroment(int* bandwidth, int bandwidth_len, char *hbw_nodes_env);

static int fill_nodes_bandwidth(int* bandwidth, int bandwidth_len);

int memkind_hbw_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

int memkind_hbw_hugetlb_check_available(struct memkind *kind)
{
    int err = memkind_hbw_check_available(kind);
    if (!err) {
        err = memkind_hugetlb_check_available_2mb(kind);
    }
    return err;
}

int memkind_hbw_gbtlb_check_available(struct memkind *kind)
{
    int err = memkind_hbw_check_available(kind);
    if (!err) {
        err = memkind_hugetlb_check_available_1gb(kind);
    }
    return err;
}

int memkind_hbw_get_mbind_nodemask(struct memkind *kind,
                                   unsigned long *nodemask,
                                   unsigned long maxnode)
{
    int cpu;
    struct bitmask nodemask_bm = {maxnode, nodemask};
    struct memkind_hbw_closest_numanode_t *g =
            &memkind_hbw_closest_numanode_g;
    pthread_once(&memkind_hbw_closest_numanode_once_g,
                 memkind_hbw_closest_numanode_init);

    if (!g->init_err && nodemask) {
        numa_bitmask_clearall(&nodemask_bm);
        cpu = sched_getcpu();
        if (cpu < g->num_cpu) {
            numa_bitmask_setbit(&nodemask_bm, g->closest_numanode[cpu]);
        }
        else {
            return MEMKIND_ERROR_GETCPU;
        }
    }
    return g->init_err;
}

int memkind_hbw_all_get_mbind_nodemask(struct memkind *kind,
                                       unsigned long *nodemask,
                                       unsigned long maxnode)
{
    int cpu;
    struct bitmask nodemask_bm = {maxnode, nodemask};
    struct memkind_hbw_closest_numanode_t *g =
            &memkind_hbw_closest_numanode_g;
    pthread_once(&memkind_hbw_closest_numanode_once_g,
                 memkind_hbw_closest_numanode_init);

    if (!g->init_err && nodemask) {
        numa_bitmask_clearall(&nodemask_bm);
        for (cpu = 0; cpu < g->num_cpu; ++cpu) {
            numa_bitmask_setbit(&nodemask_bm, g->closest_numanode[cpu]);
        }
    }
    return g->init_err;
}

static void assign_arbitrary_bandwidth_values(int* bandwidth, int bandwidth_len, struct bitmask* hbw_nodes)
{
    int i, nodes_num = numa_num_configured_nodes();

    // Assigning arbitrary bandwidth values for nodes:
    // 2 - high BW node (if bit set in hbw_nodes nodemask),
    // 1 - low  BW node,
    // 0 - node not present
    for (i=0; i<NUMA_NUM_NODES; i++) {
        if (i >= nodes_num) {
            bandwidth[i] = 0;
        }
        else if (numa_bitmask_isbitset(hbw_nodes, i)) {
            bandwidth[i] = 2;
        }
        else {
            bandwidth[i] = 1;
        }
    }

}

#define CPUID_MODEL_MASK        (0xf<<4)
#define CPUID_EXT_MODEL_MASK    (0xf<<16)

inline static void cpuid_asm(int leaf, int subleaf, uint32_t where[4])
{

    asm volatile("cpuid":"=a"(where[0]),
                         "=b"(where[1]),
                         "=c"(where[2]),
                         "=d"(where[2]):"0"(leaf), "2"(subleaf));
}

static int is_cpu_xeon_phi_x200()
{
    uint32_t xeon_phi_model = (0x7<<4);
    uint32_t xeon_phi_ext_model = (0x5<<16);
    uint32_t registers[4];
    uint32_t expected = xeon_phi_model | xeon_phi_ext_model;
    cpuid_asm(1, 0, registers);
    uint32_t actual =  registers[0] & (CPUID_MODEL_MASK | CPUID_EXT_MODEL_MASK);

    if (actual == expected) {
        return 0;
    }

    return -1;
}

///This function tries to fill bandwidth array based on knowledge about known CPU models
static int fill_bandwidth_values_heuristically(int* bandwidth, int bandwidth_len)
{
    int ret = MEMKIND_ERROR_PMTT; // Default error returned if heuristic aproach fails
    int i, nodes_num, memory_only_nodes_num = 0;
    struct bitmask *memory_only_nodes, *node_cpus;

    if (is_cpu_xeon_phi_x200() == 0) {
        nodes_num = numa_num_configured_nodes();

        // Checking if number of numa-nodes meets expectations for
        // supported configurations of Intel Xeon Phi x200
        if( nodes_num != 2 && nodes_num != 4 && nodes_num!= 8 ) {
            return ret;
        }

        memory_only_nodes = numa_allocate_nodemask();
        node_cpus = numa_allocate_nodemask();

        for(i=0; i<nodes_num; i++) {
            numa_node_to_cpus(i, node_cpus);
            if(numa_bitmask_weight(node_cpus) == 0) {
                memory_only_nodes_num++;
                numa_bitmask_setbit(memory_only_nodes, i);
            }
        }

        // Checking if number of memory-only nodes is equal number of memory+cpu nodes
        // If it pass we are changing ret to 0 (success) and filling bw table 
        if ( memory_only_nodes_num == (nodes_num - memory_only_nodes_num) ) {

            ret = 0;
            assign_arbitrary_bandwidth_values(bandwidth, bandwidth_len, memory_only_nodes);
        }

        numa_bitmask_free(memory_only_nodes);
        numa_bitmask_free(node_cpus);
    }

    return ret;
}

static int fill_bandwidth_values_from_enviroment(int* bandwidth, int bandwidth_len, char *hbw_nodes_env)
{
    struct bitmask *hbw_nodes_bm = numa_parse_nodestring(hbw_nodes_env);

    if (!hbw_nodes_bm) {
        return MEMKIND_ERROR_ENVIRON;
    }
    else {
        assign_arbitrary_bandwidth_values(bandwidth, bandwidth_len, hbw_nodes_bm);
        numa_bitmask_free(hbw_nodes_bm);
    }

    return 0;
}

static int fill_nodes_bandwidth(int* bandwidth, int bandwidth_len)
{
        char *hbw_nodes_env;
        int err = 0;

        hbw_nodes_env = getenv("MEMKIND_HBW_NODES");
        if (hbw_nodes_env) {
            return fill_bandwidth_values_from_enviroment(bandwidth, bandwidth_len, hbw_nodes_env);
        }

        err = parse_node_bandwidth(NUMA_NUM_NODES, bandwidth, MEMKIND_BANDWIDTH_PATH);
        if (err) {
            return fill_bandwidth_values_heuristically(bandwidth, bandwidth_len);
        }

        return err;
}

static void memkind_hbw_closest_numanode_init(void)
{
    struct memkind_hbw_closest_numanode_t *g =
            &memkind_hbw_closest_numanode_g;
    int *bandwidth = NULL;
    int num_unique = 0;
    int high_bandwidth = 0;

    struct bandwidth_nodes_t *bandwidth_nodes = NULL;

    g->num_cpu = numa_num_configured_cpus();
    g->closest_numanode = (int *)jemk_malloc(sizeof(int) * g->num_cpu);
    bandwidth = (int *)jemk_malloc(sizeof(int) * NUMA_NUM_NODES);

    if (!(g->closest_numanode && bandwidth)) {
        g->init_err = MEMKIND_ERROR_MALLOC;
        goto exit;
    }

    g->init_err = fill_nodes_bandwidth(bandwidth, NUMA_NUM_NODES);
    if (g->init_err)
        goto exit;

    g->init_err = create_bandwidth_nodes(NUMA_NUM_NODES, bandwidth,
                                             &num_unique, &bandwidth_nodes);
    if (g->init_err)
        goto exit;

    if (num_unique == 1) {
       g->init_err = MEMKIND_ERROR_UNAVAILABLE;
       goto exit;
    }

    high_bandwidth = bandwidth_nodes[num_unique-1].bandwidth;
    g->init_err = set_closest_numanode(num_unique, bandwidth_nodes,
                                       high_bandwidth, g->num_cpu,
                                       g->closest_numanode);

exit:

    jemk_free(bandwidth_nodes);
    jemk_free(bandwidth);

    if (g->init_err) {
        jemk_free(g->closest_numanode);
        g->closest_numanode = NULL;
    }
}

static int parse_node_bandwidth(int num_bandwidth, int *bandwidth,
                                const char *bandwidth_path)
{
    FILE *fid = NULL;
    struct stat st;
    size_t nread = 0;
    int stat_ret = 0;
    int err = 0;
    fid = fopen(bandwidth_path, "r");
    if (fid == NULL) {
        err = MEMKIND_ERROR_PMTT ? MEMKIND_ERROR_PMTT : -1;
        goto exit;
    }
    stat_ret = stat(bandwidth_path, &st);
    if (stat_ret || !st.st_size) {
        err = MEMKIND_ERROR_PMTT ? MEMKIND_ERROR_PMTT : -1;
        goto exit;
    }
    nread = fread(bandwidth, sizeof(int), num_bandwidth, fid);
    if (nread != num_bandwidth) {
        err = MEMKIND_ERROR_PMTT;
        goto exit;
    }

exit:
    if (fid != NULL) {
        fclose(fid);
    }
    return err;
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
            err = MEMKIND_ERROR_PMTT;
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
        *bandwidth_nodes = (struct bandwidth_nodes_t*)jemk_malloc(
                               sizeof(struct bandwidth_nodes_t) **num_unique +
                               sizeof(int) * num_bandwidth);
        if (!*bandwidth_nodes) {
            err = MEMKIND_ERROR_MALLOC;
        }
    }
    if (!err) {
        /* populate output */
        (*bandwidth_nodes)[0].numanodes = (int*)(*bandwidth_nodes + *num_unique);
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
        err = MEMKIND_ERROR_PMTT;
    }
    else {
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
                }
                else if (distance == min_distance) {
                    min_unique = 0;
                }
            }
            if (!min_unique) {
                err = MEMKIND_ERROR_TIEDISTANCE;
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

void memkind_hbw_init_once(void)
{
    int err = memkind_arena_create_map(MEMKIND_HBW);
    assert(err == 0);
    err = numa_available();
    assert(err == 0);
}

void memkind_hbw_hugetlb_init_once(void)
{
    int err = memkind_arena_create_map(MEMKIND_HBW_HUGETLB);
    assert(err == 0);
    err = numa_available();
    assert(err == 0);
}

void memkind_hbw_preferred_init_once(void)
{
    int err = memkind_arena_create_map(MEMKIND_HBW_PREFERRED);
    assert(err == 0);
    err = numa_available();
    assert(err == 0);
}

void memkind_hbw_preferred_hugetlb_init_once(void)
{
    int err = memkind_arena_create_map(MEMKIND_HBW_PREFERRED_HUGETLB);
    assert(err == 0);
    err = numa_available();
    assert(err == 0);
}

void memkind_hbw_interleave_init_once(void)
{
    int err = memkind_arena_create_map(MEMKIND_HBW_INTERLEAVE);
    assert(err == 0);
    err = numa_available();
    assert(err == 0);
}
