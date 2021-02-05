// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_mem_attributes.h>
#include <memkind/internal/memkind_private.h>

#include "config.h"

#ifdef MEMKIND_HWLOC

#include <memkind/internal/vec.h>

#include <hwloc.h>
#include <hwloc/linux-libnuma.h>
#define MEMKIND_HBW_THRESHOLD_DEFAULT                                          \
    (200 * 1024) // Default threshold is 200 GB/s

int get_per_cpu_local_nodes_mask(struct bitmask ***nodes_mask,
                                 memkind_node_variant_t node_variant,
                                 memory_attribute_t attr)
{
    int num_nodes = numa_num_configured_nodes();
    int num_cpus = numa_num_configured_cpus();

    hwloc_obj_t init_node = NULL;
    hwloc_obj_t *local_nodes = NULL;
    hwloc_cpuset_t node_cpus = NULL;
    hwloc_nodeset_t attr_loc_mask = NULL;
    hwloc_uint64_t mem_attr, best_mem_attr;
    unsigned int mem_attr_nodes;

    hwloc_topology_t topology;
    int i;
    int ret;

    int err = hwloc_topology_init(&topology);
    if (MEMKIND_UNLIKELY(err)) {
        log_err("hwloc initialization failed");
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    err = hwloc_topology_load(topology);
    if (MEMKIND_UNLIKELY(err)) {
        log_err("hwloc topology load failed");
        hwloc_topology_destroy(topology);
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    *nodes_mask = calloc(sizeof(struct bitmask *), num_cpus);
    if (MEMKIND_UNLIKELY(*nodes_mask == NULL)) {
        log_err("calloc() failed.");
        hwloc_topology_destroy(topology);
        return MEMKIND_ERROR_MALLOC;
    }

    local_nodes = malloc(sizeof(hwloc_obj_t) * num_nodes);
    if (MEMKIND_UNLIKELY(local_nodes == NULL)) {
        ret = MEMKIND_ERROR_MALLOC;
        log_err("malloc() failed.");
        goto error;
    }

    node_cpus = hwloc_bitmap_alloc();
    if (MEMKIND_UNLIKELY(node_cpus == NULL)) {
        ret = MEMKIND_ERROR_MALLOC;
        log_err("hwloc_bitmap_alloc() failed.");
        goto error;
    }

    attr_loc_mask = hwloc_bitmap_alloc();
    if (MEMKIND_UNLIKELY(attr_loc_mask == NULL)) {
        ret = MEMKIND_ERROR_MALLOC;
        log_err("hwloc_bitmap_alloc() failed.");
        goto error;
    }

    // iterate over all NUMA nodes
    while ((init_node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                   init_node)) != NULL) {

        // skip this node if it doesn't contain any CPU
        if (hwloc_bitmap_isincluded(init_node->cpuset, node_cpus)) {
            log_info("Node %d skipped - no CPU detected in initiator Node.",
                     init_node->os_index);
            continue;
        }
        hwloc_bitmap_or(node_cpus, node_cpus, init_node->cpuset);

        // extract local nodes
        struct hwloc_location initiator;
        initiator.type = HWLOC_LOCATION_TYPE_CPUSET;
        initiator.location.cpuset = init_node->cpuset;
        unsigned int num_local_nodes = num_nodes;
        err = hwloc_get_local_numanode_objs(topology, &initiator,
                                            &num_local_nodes, local_nodes, 0);
        if (err) {
            log_err("hwloc_get_local_numanode_objs");
            ret = MEMKIND_ERROR_UNAVAILABLE;
            goto error;
        }

        hwloc_bitmap_zero(attr_loc_mask);
        switch (attr) {
            case MEM_ATTR_CAPACITY:
                best_mem_attr = 0;
                for (i = 0; i < num_local_nodes; ++i) {
                    if (local_nodes[i]->attr->numanode.local_memory == 0) {
                        log_info("Node skipped - Node %d has no memory.", i);
                        continue;
                    }

                    if (local_nodes[i]->attr->numanode.local_memory >
                        best_mem_attr) {
                        best_mem_attr =
                            local_nodes[i]->attr->numanode.local_memory;
                        hwloc_bitmap_only(attr_loc_mask,
                                          local_nodes[i]->os_index);
                    } else if (local_nodes[i]->attr->numanode.local_memory ==
                               best_mem_attr) {
                        hwloc_bitmap_set(attr_loc_mask,
                                         local_nodes[i]->os_index);
                    }
                }
                break;

            case MEM_ATTR_BANDWIDTH:
                best_mem_attr = 0;
                for (i = 0; i < num_local_nodes; ++i) {
                    err = hwloc_memattr_get_value(
                        topology, HWLOC_MEMATTR_ID_BANDWIDTH, local_nodes[i],
                        &initiator, 0, &mem_attr);
                    if (err) {
                        log_info(
                            "Node skipped - cannot read initiator Node %d and target Node %d.",
                            init_node->os_index, local_nodes[i]->os_index);
                        continue;
                    }

                    if (mem_attr > best_mem_attr) {
                        best_mem_attr = mem_attr;
                        hwloc_bitmap_only(attr_loc_mask,
                                          local_nodes[i]->os_index);
                    } else if (mem_attr == best_mem_attr) {
                        hwloc_bitmap_set(attr_loc_mask,
                                         local_nodes[i]->os_index);
                    }
                }
                break;

            case MEM_ATTR_LATENCY:
                best_mem_attr = INT_MAX;
                for (i = 0; i < num_local_nodes; ++i) {
                    err = hwloc_memattr_get_value(
                        topology, HWLOC_MEMATTR_ID_LATENCY, local_nodes[i],
                        &initiator, 0, &mem_attr);
                    if (err) {
                        log_info(
                            "Node skipped - cannot read initiator Node %d and target Node %d.",
                            init_node->os_index, local_nodes[i]->os_index);
                        continue;
                    }

                    if (mem_attr < best_mem_attr) {
                        best_mem_attr = mem_attr;
                        hwloc_bitmap_only(attr_loc_mask,
                                          local_nodes[i]->os_index);
                    } else if (mem_attr == best_mem_attr) {
                        hwloc_bitmap_set(attr_loc_mask,
                                         local_nodes[i]->os_index);
                    }
                }
                break;

            default:
                log_err("Unknown memory attribute.");
                ret = MEMKIND_ERROR_UNAVAILABLE;
                goto error;
        }

        mem_attr_nodes = hwloc_bitmap_weight(attr_loc_mask);

        if (node_variant == NODE_VARIANT_SINGLE && mem_attr_nodes > 1) {
            ret = MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
            log_err(
                "Multiple NUMA Nodes have the same value of memory attribute for init node %d.",
                init_node->os_index);
            goto error;
        }

        if (mem_attr_nodes == 0) {
            ret = MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
            log_err("No memory attribute Nodes for init node %d.",
                    init_node->os_index);
            goto error;
        }

        // populate memory attribute nodemask to all CPU's from initiator NUMA
        // node
        hwloc_bitmap_foreach_begin(i, init_node->cpuset)(*nodes_mask)[i] =
            hwloc_nodeset_to_linux_libnuma_bitmask(topology, attr_loc_mask);
        if (MEMKIND_UNLIKELY((*nodes_mask)[i] == NULL)) {
            ret = MEMKIND_ERROR_MALLOC;
            log_err("hwloc_nodeset_to_linux_libnuma_bitmask() failed.");
            goto error;
        }
        hwloc_bitmap_foreach_end();
    }

    ret = MEMKIND_SUCCESS;
    goto success;

error:
    for (i = 0; i < num_cpus; ++i) {
        if ((*nodes_mask)[i]) {
            numa_bitmask_free((*nodes_mask)[i]);
        }
    }
    free(*nodes_mask);

success:
    hwloc_bitmap_free(attr_loc_mask);
    hwloc_bitmap_free(node_cpus);
    free(local_nodes);
    hwloc_topology_destroy(topology);

    return ret;
}

// Vector of CPUs with memory NUMA Node id(s)
VEC(vec_cpu_node, int);

int set_closest_numanode_mem_attr(void **numanode,
                                  memkind_node_variant_t node_variant)
{
    const char *hbw_threshold_env = memkind_get_env("MEMKIND_HBW_THRESHOLD");
    int num_cpu = numa_num_configured_cpus();
    size_t hbw_threshold, vec_size;
    int err, i, status;
    hwloc_topology_t topology;
    hwloc_obj_t init_node = NULL;
    hwloc_cpuset_t node_cpus = NULL;

    if (hbw_threshold_env) {
        log_info("Environment variable MEMKIND_HBW_THRESHOLD detected: %s.",
                 hbw_threshold_env);
        char *end;
        errno = 0;
        hbw_threshold = strtoull(hbw_threshold_env, &end, 10);
        if (hbw_threshold > UINT_MAX || *end != '\0' || errno != 0) {
            log_err(
                "Environment variable MEMKIND_HBW_THRESHOLD is invalid value.");
            return MEMKIND_ERROR_ENVIRON;
        }
    } else {
        hbw_threshold = MEMKIND_HBW_THRESHOLD_DEFAULT;
    }

    err = hwloc_topology_init(&topology);
    if (MEMKIND_UNLIKELY(err)) {
        log_err("hwloc initialize failed");
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    err = hwloc_topology_load(topology);
    if (MEMKIND_UNLIKELY(err)) {
        log_err("hwloc topology load failed");
        status = MEMKIND_ERROR_UNAVAILABLE;
        goto hwloc_destroy;
    }

    node_cpus = hwloc_bitmap_alloc();
    if (MEMKIND_UNLIKELY(node_cpus == NULL)) {
        log_err("hwloc_bitmap_alloc failed");
        status = MEMKIND_ERROR_UNAVAILABLE;
        goto hwloc_destroy;
    }

    VEC(vec_temp, int) current_dest_nodes = VEC_INITIALIZER;

    struct vec_cpu_node *node_arr =
        (struct vec_cpu_node *)calloc(num_cpu, sizeof(struct vec_cpu_node));

    if (MEMKIND_UNLIKELY(node_arr == NULL)) {
        log_err("calloc failed");
        status = MEMKIND_ERROR_MALLOC;
        goto node_cpu_free;
    }

    while ((init_node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                   init_node)) != NULL) {
        struct hwloc_location initiator;
        hwloc_obj_t target = NULL;
        int min_distance = INT_MAX;

        // skip node which could not be a initiator
        if (hwloc_bitmap_isincluded(init_node->cpuset, node_cpus)) {
            log_info("Node %d skipped - no CPU detected in initiator Node.",
                     init_node->os_index);
            continue;
        }

        hwloc_bitmap_or(node_cpus, node_cpus, init_node->cpuset);

        initiator.type = HWLOC_LOCATION_TYPE_CPUSET;
        initiator.location.cpuset = init_node->cpuset;

        VEC_CLEAR(&current_dest_nodes);

        while ((target = hwloc_get_next_obj_by_type(
                    topology, HWLOC_OBJ_NUMANODE, target)) != NULL) {
            hwloc_uint64_t bandwidth;
            err = hwloc_memattr_get_value(topology, HWLOC_MEMATTR_ID_BANDWIDTH,
                                          target, &initiator, 0, &bandwidth);
            if (err) {
                log_info(
                    "Node skipped - cannot read initiator Node %d and target Node %d.",
                    init_node->os_index, target->os_index);
                continue;
            }

            if (bandwidth >= hbw_threshold) {
                if (node_variant == NODE_VARIANT_ALL) {
                    VEC_PUSH_BACK(&current_dest_nodes, target->os_index);
                } else {
                    int dist =
                        numa_distance(init_node->os_index, target->os_index);
                    if (dist < min_distance) {
                        min_distance = dist;
                        VEC_CLEAR(&current_dest_nodes);
                        VEC_PUSH_BACK(&current_dest_nodes, target->os_index);
                    } else if (dist == min_distance) {
                        VEC_PUSH_BACK(&current_dest_nodes, target->os_index);
                    }
                }
            }
        }

        vec_size = VEC_SIZE(&current_dest_nodes);

        if (vec_size == 0) {
            log_err("No HBW Nodes for init node %d.", init_node->os_index);
            status = MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
            goto free_node_arr;
        }

        // validate single NUMA Node condition
        if (node_variant == NODE_VARIANT_SINGLE && vec_size > 1) {
            log_err("Invalid Numa Configuration for Node %d",
                    init_node->os_index);
            status = MEMKIND_ERROR_RUNTIME;
            goto free_node_arr;
        }

        // populate memory attribute nodemask to all CPU's from initiator NUMA
        // node
        hwloc_bitmap_foreach_begin(i, init_node->cpuset) int node = -1;
        VEC_FOREACH(node, &current_dest_nodes)
        {
            VEC_PUSH_BACK(&node_arr[i], node);
        }
        hwloc_bitmap_foreach_end();
    }

    *numanode = node_arr;
    status = MEMKIND_SUCCESS;
    goto free_current_dest_nodes;

free_node_arr:
    for (i = 0; i < num_cpu; ++i) {
        if (VEC_CAPACITY(&node_arr[i]))
            VEC_DELETE(&node_arr[i]);
    }
    free(node_arr);

free_current_dest_nodes:
    VEC_DELETE(&current_dest_nodes);

node_cpu_free:
    hwloc_bitmap_free(node_cpus);

hwloc_destroy:
    hwloc_topology_destroy(topology);

    return status;
}
#else
int get_per_cpu_local_nodes_mask(struct bitmask ***nodes_mask,
                                 memkind_node_variant_t node_variant,
                                 memory_attribute_t attr)
{
    log_err("Memory attribute NUMA nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
}

int set_closest_numanode_mem_attr(void **numanode,
                                  memkind_node_variant_t node_variant)
{
    log_err("High Bandwidth NUMA nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
}
#endif
