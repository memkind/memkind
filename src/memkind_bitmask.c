// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2021 Intel Corporation. */

#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include <memkind/internal/memkind_bitmask.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/vec.h>

// Vector of CPUs with memory NUMA Node id(s)
VEC(vec_cpu_node, int);

int memkind_env_get_nodemask(char *nodes_env, struct bitmask **bm)
{
    *bm = numa_parse_nodestring(nodes_env);
    if (*bm == NULL) {
        log_err("Invalid value of environment variable %s.", nodes_env);
        return MEMKIND_ERROR_ENVIRON;
    }
    return MEMKIND_SUCCESS;
}

int set_closest_numanode(get_node_bitmask get_bitmask, void **numanode,
                         memkind_node_variant_t node_variant)
{
    int num_cpu = numa_num_configured_cpus();
    struct bitmask *dest_nodes_mask = NULL;
    int status = get_bitmask(&dest_nodes_mask);
    if (status)
        return status;

    struct bitmask *node_cpu_mask = numa_allocate_cpumask();
    if (!node_cpu_mask) {
        status = MEMKIND_ERROR_MALLOC;
        log_err("numa_allocate_cpumask() failed.");
        goto free_dest_nodemask;
    }

    VEC(vec_temp, int) current_dest_nodes = VEC_INITIALIZER;

    struct vec_cpu_node *node_arr =
        (struct vec_cpu_node *)calloc(num_cpu, sizeof(struct vec_cpu_node));
    if (!node_arr) {
        status = MEMKIND_ERROR_MALLOC;
        log_err("calloc() failed.");
        goto free_cpu_mask;
    }

    int init_node, dest_node, cpu_id;
    int max_node_id = numa_max_node();

    for (init_node = 0; init_node <= max_node_id; ++init_node) {

        // skip missing node and node which could not be a initiator
        if ((numa_node_to_cpus(init_node, node_cpu_mask) != 0) ||
            numa_bitmask_weight(node_cpu_mask) == 0) {
            continue;
        }

        // find closest NUMA Node(s) from destnation NUMA Node(s)
        int min_distance = INT_MAX;
        VEC_CLEAR(&current_dest_nodes);
        for (dest_node = 0; dest_node <= max_node_id; ++dest_node) {
            if (numa_bitmask_isbitset(dest_nodes_mask, dest_node)) {
                if (node_variant == NODE_VARIANT_ALL) {
                    VEC_PUSH_BACK(&current_dest_nodes, dest_node);
                } else {
                    int old_errno = errno;
                    int dist = numa_distance(init_node, dest_node);
                    errno = old_errno;
                    if (dist < min_distance) {
                        min_distance = dist;
                        VEC_CLEAR(&current_dest_nodes);
                        VEC_PUSH_BACK(&current_dest_nodes, dest_node);
                    } else if (dist == min_distance) {
                        VEC_PUSH_BACK(&current_dest_nodes, dest_node);
                    }
                }
            }
        }

        // validate single NUMA Node condition
        if ((node_variant == NODE_VARIANT_SINGLE) &&
            (VEC_SIZE(&current_dest_nodes) > 1)) {
            log_err("Invalid Numa Configuration for Node %d", init_node);
            status = MEMKIND_ERROR_RUNTIME;
            for (cpu_id = 0; cpu_id < num_cpu; ++cpu_id) {
                if (VEC_CAPACITY(&node_arr[cpu_id]))
                    VEC_DELETE(&node_arr[cpu_id]);
            }
            goto free_node_arr;
        }

        // fill mapping CPU destination memory Node(s)
        for (cpu_id = 0; cpu_id < num_cpu; ++cpu_id) {
            if (numa_bitmask_isbitset(node_cpu_mask, cpu_id)) {
                int node = -1;
                VEC_FOREACH(node, &current_dest_nodes)
                {
                    VEC_PUSH_BACK(&node_arr[cpu_id], node);
                }
            }
        }
    }

    *numanode = node_arr;
    status = MEMKIND_SUCCESS;
    goto free_current_dest_nodes;

free_node_arr:
    free(node_arr);

free_current_dest_nodes:
    VEC_DELETE(&current_dest_nodes);

free_cpu_mask:
    numa_bitmask_free(node_cpu_mask);

free_dest_nodemask:
    numa_bitmask_free(dest_nodes_mask);

    return status;
}

int set_bitmask_for_current_numanode(unsigned long *nodemask,
                                     unsigned long maxnode,
                                     const void *numanode)
{
    if (MEMKIND_LIKELY(nodemask)) {
        struct bitmask nodemask_bm = {maxnode, nodemask};
        numa_bitmask_clearall(&nodemask_bm);
        int cpu = sched_getcpu();
        int node = -1;
        const struct vec_cpu_node *closest_numanode_vec =
            (const struct vec_cpu_node *)numanode;
        VEC_FOREACH(node, &closest_numanode_vec[cpu])
        {
            numa_bitmask_setbit(&nodemask_bm, node);
        }
    }
    return MEMKIND_SUCCESS;
}
