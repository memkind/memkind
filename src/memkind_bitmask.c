// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/memkind_bitmask.h>

VEC(vec_cpu_node, int);
VEC(vec_tmp, int);

struct bitmask *memkind_env_get_nodemask(char *nodes_env)
{
    struct bitmask *numa_nodes_bm = numa_parse_nodestring(nodes_env);
    if (!numa_nodes_bm)
        log_err("Invalid value of environment variable %s.", nodes_env);
    return numa_nodes_bm;
}

int set_closest_numanode(get_node_bitmask get_bitmask,
                         struct vec_cpu_node **closest_numanode, int num_cpu, bool is_single_node)
{
    int status;
    struct bitmask *dest_nodes_mask = get_bitmask();
    if (!dest_nodes_mask)
        return MEMKIND_ERROR_UNAVAILABLE;

    struct bitmask *node_cpu_mask = numa_allocate_cpumask();
    if (!node_cpu_mask) {
        status = MEMKIND_ERROR_MALLOC;
        log_err("malloc() failed.");
        goto free_dest_nodemask;
    }

    struct vec_tmp *vec_temp = (struct vec_tmp *) calloc(1, sizeof(struct vec_tmp));
    if (!vec_temp) {
        status = MEMKIND_ERROR_MALLOC;
        log_err("malloc() failed.");
        goto free_cpu_mask;
    }
    struct vec_cpu_node *node_arr = (struct vec_cpu_node *) calloc(num_cpu,
                                                                   sizeof(struct vec_cpu_node));
    if (!node_arr) {
        status = MEMKIND_ERROR_MALLOC;
        log_err("malloc() failed.");
        goto free_vec_temp;
    }

    int init_node;
    int dest_node;
    int cpu_id;
    int max_node = numa_max_node();

    for (init_node = 0; init_node <= max_node; ++init_node) {

        //skip missing node and node which could not be a initiator
        if((numa_node_to_cpus(init_node, node_cpu_mask) != 0) ||
           numa_bitmask_weight(node_cpu_mask) == 0) {
            continue;
        }

        int min_distance = INT_MAX;
        VEC_CLEAR(vec_temp);
        for (dest_node = 0; dest_node <= max_node; ++dest_node) {
            if(numa_bitmask_isbitset(dest_nodes_mask, dest_node)) {
                int old_errno = errno;
                int dist = numa_distance(init_node, dest_node);
                errno = old_errno;
                if (dist < min_distance) {
                    min_distance = dist;
                    VEC_CLEAR(vec_temp);
                    VEC_PUSH_BACK(vec_temp, dest_node);
                } else if (dist == min_distance) {
                    VEC_PUSH_BACK(vec_temp, dest_node);
                }
            }
        }

        if (is_single_node && VEC_SIZE(vec_temp)) {
            log_err("Invalid Numa Configuration for Node %d", init_node);
            status =  MEMKIND_ERROR_RUNTIME;
            for (cpu_id = 0; cpu_id < num_cpu; ++cpu_id) {
                //TODO check this vector free some garbage??
                VEC_DELETE(&node_arr[cpu_id]);
            }
            goto free_node_arr;
        }

        for (cpu_id = 0; cpu_id < num_cpu; ++cpu_id) {
            if(numa_bitmask_isbitset(node_cpu_mask, cpu_id)) {
                int node_test = -1;
                VEC_FOREACH(node_test, vec_temp) {
                    VEC_PUSH_BACK(&node_arr[cpu_id], node_test);
                }
            }
        }
    }

    *closest_numanode = node_arr;
    status = MEMKIND_SUCCESS;
    goto free_vec_temp;

free_node_arr:
    free(node_arr);

free_vec_temp:
    free(vec_temp);

free_cpu_mask:
    numa_bitmask_free(node_cpu_mask);

free_dest_nodemask:
    numa_bitmask_free(dest_nodes_mask);

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
