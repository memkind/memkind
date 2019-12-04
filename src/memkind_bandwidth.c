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

#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/memkind_bandwidth.h>

struct numanode_bandwidth_t {
    int numanode;
    int bandwidth;
};

struct bandwidth_nodes_t {
    int bandwidth;
    int num_numanodes;
    int *numanodes;
};

VEC(vec_cpu_node, int);

#define BANDWIDTH_NODE_HIGH_VAL       2
#define BANDWIDTH_NODE_LOW_VAL        1
#define BANDWIDTH_NODE_NOT_PRESENT    0

static void bandwidth_assign_arbitrary_values(int *bandwidth,
                                              struct bitmask *numa_nodes_bm)
{
    unsigned i;
    int nodes_num = numa_num_configured_nodes();

    for (i = 0; i<NUMA_NUM_NODES; i++) {
        if (numa_bitmask_isbitset(numa_nodes_bm, i)) {
            bandwidth[i] = BANDWIDTH_NODE_HIGH_VAL;
            nodes_num--;
        }
    }

    for (i = 0; i<NUMA_NUM_NODES; i++) {
        if (!numa_bitmask_isbitset(numa_nodes_bm, i)) {
            bandwidth[i] = BANDWIDTH_NODE_LOW_VAL;
            nodes_num--;
        }
        if (nodes_num==0) break;
    }
}

int bandwidth_fill(int *bandwidth, get_node_bitmask get_bitmask)
{
    struct bitmask *high_val_node_mask = numa_allocate_nodemask();
    int ret = get_bitmask(high_val_node_mask);
    if(ret == 0) {
        bandwidth_assign_arbitrary_values(bandwidth, high_val_node_mask);
    }
    numa_bitmask_free(high_val_node_mask);

    return ret;
}

static int bandwidth_fill_values_from_enviroment(int *bandwidth,
                                                 char *nodes_env)
{
    struct bitmask *numa_nodes_bm = numa_parse_nodestring(nodes_env);

    if (!numa_nodes_bm) {
        log_err("Invalid value of environment variable.");
        return MEMKIND_ERROR_ENVIRON;
    } else {
        bandwidth_assign_arbitrary_values(bandwidth, numa_nodes_bm);
        numa_bitmask_free(numa_nodes_bm);
    }

    return 0;
}

static int bandwidth_fill_nodes(int *bandwidth, fill_bandwidth_values fill,
                                const char *env)
{
    char *high_value_nodes_env = secure_getenv(env);
    if (high_value_nodes_env) {
        log_info("Environment variable %s detected: %s.", env, high_value_nodes_env);
        return bandwidth_fill_values_from_enviroment(bandwidth, high_value_nodes_env);
    }
    return fill(bandwidth);
}

static int bandwidth_compare_numanode(const void *a, const void *b)
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

static int bandwidth_create_nodes(const int *bandwidth, int *num_unique,
                                  struct bandwidth_nodes_t **bandwidth_nodes)
{
    /***************************************************************************
    *   bandwidth (IN):                                                        *
    *       A vector of length equal NUMA_NUM_NODES that gives bandwidth for   *
    *       each numa node, zero if numa node has unknown bandwidth.           *
    *   num_unique (OUT):                                                      *
    *       number of unique non-zero bandwidth values in bandwidth            *
    *       vector.                                                            *
    *   bandwidth_nodes (OUT):                                                 *
    *       A list of length num_unique sorted by bandwidth value where        *
    *       each element gives a list of the numa nodes that have the          *
    *       given bandwidth.                                                   *
    *   RETURNS MEMKIND_SUCCESS on success, error code on failure              *
    ***************************************************************************/
    int err = MEMKIND_SUCCESS;
    int i, last_bandwidth;
    *bandwidth_nodes = NULL;
    /* allocate space for sorting array */
    int nodes_num = numa_num_configured_nodes();
    struct numanode_bandwidth_t *numanode_bandwidth = malloc(sizeof(
                                                                 struct numanode_bandwidth_t) * nodes_num);
    if (!numanode_bandwidth) {
        err = MEMKIND_ERROR_MALLOC;
        log_err("malloc() failed.");
    }
    if (!err) {
        /* set sorting array */
        int j = 0;
        for (i = 0; i < NUMA_NUM_NODES; ++i) {
            if (bandwidth[i] != BANDWIDTH_NODE_NOT_PRESENT) {
                numanode_bandwidth[j].numanode = i;
                numanode_bandwidth[j].bandwidth = bandwidth[i];
                ++j;
            }
        }

        if (j == 0) {
            err = MEMKIND_ERROR_UNAVAILABLE;
        }
    }
    if (!err) {
        qsort(numanode_bandwidth, nodes_num, sizeof(struct numanode_bandwidth_t),
              bandwidth_compare_numanode);
        /* calculate the number of unique bandwidths */
        *num_unique = 1;
        last_bandwidth = numanode_bandwidth[0].bandwidth;
        for (i = 1; i < nodes_num; ++i) {
            if (numanode_bandwidth[i].bandwidth != last_bandwidth) {
                last_bandwidth = numanode_bandwidth[i].bandwidth;
                ++*num_unique;
            }
        }
        /* allocate output array */
        *bandwidth_nodes = (struct bandwidth_nodes_t *)malloc(sizeof(
                                                                  struct bandwidth_nodes_t) * (*num_unique) + sizeof(int) * nodes_num);
        if (!*bandwidth_nodes) {
            err = MEMKIND_ERROR_MALLOC;
            log_err("malloc() failed.");
        }
    }
    if (!err) {
        /* populate output */
        (*bandwidth_nodes)[0].numanodes = (int *)(*bandwidth_nodes + *num_unique);
        last_bandwidth = numanode_bandwidth[0].bandwidth;
        int k = 0;
        int l = 0;
        for (i = 0; i < nodes_num; ++i, ++l) {
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
        free(numanode_bandwidth);
    }
    if (err) {
        if (*bandwidth_nodes) {
            free(*bandwidth_nodes);
        }
    }
    return err;
}

static int bandwidth_set_closest_numanode(int num_unique,
                                          const struct bandwidth_nodes_t *bandwidth_nodes, bool is_single_node,
                                          int num_cpunode, struct vec_cpu_node **closest_numanode)
{
    /***************************************************************************
    *   num_unique (IN):                                                       *
    *       Length of bandwidth_nodes vector.                                  *
    *   bandwidth_nodes (IN):                                                  *
    *       Output vector from bandwidth_create_nodes().                       *
    *   is_single_node (IN):                                                   *
    *       Flag used to mark is only single closest numanode                  *
    *       is a valid configuration                                           *
    *   num_cpunode (IN):                                                      *
    *       Number of cpu's and length of closest_numanode.                    *
    *   closest_numanode (OUT):                                                *
    *       Vector that maps cpu index to closest numa node(s)                 *
    *       of the specified bandwidth.                                        *
    *   RETURNS zero on success, error code on failure                         *
    ***************************************************************************/
    int err = MEMKIND_SUCCESS;
    int min_distance, distance, i, j, old_errno;
    struct bandwidth_nodes_t match;
    match.bandwidth = -1;
    int target_bandwidth = bandwidth_nodes[num_unique-1].bandwidth;

    struct vec_cpu_node *node_arr = (struct vec_cpu_node *) calloc(num_cpunode,
                                                                   sizeof(struct vec_cpu_node));

    if (!node_arr) {
        log_err("calloc() failed.");
        return MEMKIND_ERROR_MALLOC;
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
            for (j = 0; j < match.num_numanodes; ++j) {
                old_errno = errno;
                distance = numa_distance(numa_node_of_cpu(i), match.numanodes[j]);
                errno = old_errno;
                if (distance < min_distance) {
                    min_distance = distance;
                    VEC_CLEAR(&node_arr[i]);
                    VEC_PUSH_BACK(&node_arr[i], match.numanodes[j]);
                } else if (distance == min_distance) {
                    VEC_PUSH_BACK(&node_arr[i], match.numanodes[j]);
                }
            }
            if (VEC_SIZE(&node_arr[i]) > 1 && is_single_node &&
                numa_bitmask_isbitset(numa_all_cpus_ptr, i)) {
                log_err("Invalid Numa Configuration for cpu %d", i);
                int node = -1;
                VEC_FOREACH(node, &node_arr[i]) {
                    log_err("Node %d", node);
                }
                err = MEMKIND_ERROR_RUNTIME;
            }
        }
    }

    if (err) {
        for (i = 0; i < num_cpunode; ++i) {
            VEC_DELETE(&node_arr[i]);
        }
        free(node_arr);
        node_arr = NULL;
    }

    *closest_numanode = node_arr;

    return err;
}

int set_closest_numanode(fill_bandwidth_values fill_values, const char *env,
                         struct vec_cpu_node **closest_numanode, int num_cpu, bool is_single_node)
{
    int status;
    int num_unique = 0;
    int i;
    struct bandwidth_nodes_t *bandwidth_nodes = NULL;
    int *bandwidth = (int *)calloc(NUMA_NUM_NODES, sizeof(int));

    if (!bandwidth) {
        log_err("calloc() failed.");
        return MEMKIND_ERROR_MALLOC;
    }

    status = bandwidth_fill_nodes(bandwidth, fill_values, env);
    if (status)
        goto exit;

    status = bandwidth_create_nodes(bandwidth, &num_unique, &bandwidth_nodes);
    if (status)
        goto exit;

    status = bandwidth_set_closest_numanode(num_unique, bandwidth_nodes,
                                            is_single_node, num_cpu, closest_numanode);

    for(i = 0; i < bandwidth_nodes[num_unique-1].num_numanodes; ++i) {
        log_info("NUMA node %d is high-bandwidth/dax-kmem memory.",
                 bandwidth_nodes[num_unique-1].numanodes[i]);
    }

exit:

    free(bandwidth_nodes);
    free(bandwidth);

    return status;
}

void set_bitmask_for_all_closest_numanodes(unsigned long *nodemask,
                                           unsigned long maxnode, const struct vec_cpu_node *closest_numanode, int num_cpu)
{
    if (MEMKIND_LIKELY(nodemask)) {
        struct bitmask nodemask_bm = {maxnode, nodemask};
        int cpu;
        int node = -1;
        numa_bitmask_clearall(&nodemask_bm);
        for (cpu = 0; cpu < num_cpu; ++cpu) {
            VEC_FOREACH(node, &closest_numanode[cpu]) {
                numa_bitmask_setbit(&nodemask_bm, node);
            }
        }
    }
}

int set_bitmask_for_current_closest_numanode(unsigned long *nodemask,
                                             unsigned long maxnode, const struct vec_cpu_node *closest_numanode, int num_cpu)
{
    if (MEMKIND_LIKELY(nodemask)) {
        struct bitmask nodemask_bm = {maxnode, nodemask};
        numa_bitmask_clearall(&nodemask_bm);
        int cpu = sched_getcpu();
        if (MEMKIND_LIKELY(cpu < num_cpu)) {
            int node = -1;
            VEC_FOREACH(node, &closest_numanode[cpu]) {
                numa_bitmask_setbit(&nodemask_bm, node);
            }
        } else {
            return MEMKIND_ERROR_RUNTIME;
        }
    }
    return MEMKIND_SUCCESS;
}
