// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#include <memkind/internal/memkind_mem_attributes.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>

#include "config.h"

#ifdef MEMKIND_HWLOC
#include <hwloc.h>
#define MEMKIND_HBW_THRESHOLD_DEFAULT (200 * 1024) // Default threshold is 200 GB/s

int get_per_cpu_hi_cap_local_nodes_mask(struct bitmask ***nodes_mask,
                                        int node_variant)
{
    int num_nodes = numa_num_configured_nodes();
    int max_node_id = numa_max_node();
    int num_cpus = numa_num_configured_cpus();

    hwloc_obj_t init_node = NULL;
    hwloc_obj_t *local_nodes = NULL;
    struct bitmask *node_cpus = NULL;

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
        ret = MEMKIND_ERROR_MALLOC;
        log_err("calloc() failed.");
        goto error;
    }

    local_nodes = malloc(sizeof(hwloc_obj_t) * num_nodes);
    if (MEMKIND_UNLIKELY(local_nodes == NULL)) {
        ret = MEMKIND_ERROR_MALLOC;
        log_err("malloc() failed.");
        goto error;
    }

    node_cpus = numa_allocate_cpumask();
    if (MEMKIND_UNLIKELY(node_cpus == NULL)) {
        ret = MEMKIND_ERROR_MALLOC;
        log_err("numa_allocate_cpumask() failed.");
        goto error;
    }

    struct bitmask *hi_cap_loc_mask = numa_bitmask_alloc(max_node_id + 1);
    if (MEMKIND_UNLIKELY(hi_cap_loc_mask == NULL)) {
        ret = MEMKIND_ERROR_MALLOC;
        log_err("numa_allocate_nodemask() failed.");
        goto error;
    }

    // iterate over all NUMA nodes
    while ((init_node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                   init_node)) != NULL) {

        // skip this node if it doesn't contain any CPU
        numa_node_to_cpus(init_node->os_index, node_cpus);
        if (numa_bitmask_weight(node_cpus) == 0) {
            log_info("Node %d skipped - no CPU detected in initiator Node.",
                     init_node->os_index);
            continue;
        }

        // fill "local_nodes" array
        struct hwloc_location initiator;
        initiator.type = HWLOC_LOCATION_TYPE_OBJECT;
        initiator.location.object = init_node;
        unsigned int num_local_nodes = num_nodes;
        err = hwloc_get_local_numanode_objs(topology, &initiator, &num_local_nodes,
                                            local_nodes, 0);
        if (err) {
            log_err("hwloc_get_local_numanode_objs");
            ret = MEMKIND_ERROR_UNAVAILABLE;
            goto error;
        }

        // find highest capacity nodes among nodes in "local_nodes" array
        long long best_capacity = 0;
        numa_bitmask_clearall(hi_cap_loc_mask);
        for (i = 0; i < num_local_nodes; ++i) {
            long long current_capacity = numa_node_size64(local_nodes[i]->os_index, NULL);
            if (current_capacity == -1) {
                continue;
            }

            if (current_capacity > best_capacity) {
                best_capacity = current_capacity;
                numa_bitmask_clearall(hi_cap_loc_mask);
            }

            if (current_capacity == best_capacity) {
                numa_bitmask_setbit(hi_cap_loc_mask, local_nodes[i]->os_index);
            }
        }

        if (node_variant == NODE_VARIANT_SINGLE &&
            numa_bitmask_weight(hi_cap_loc_mask) > 1) {
            ret = MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
            log_err("Multiple NUMA Nodes have the same highest capacity local.");
            goto error;
        }

        // copy bitmask with Highest Local Capacty NUMA nodes
        // to all CPUs from innitiator node
        for (i = 0; i < num_cpus; ++i) {
            if (numa_bitmask_isbitset(node_cpus, i)) {
                (*nodes_mask)[i] = numa_bitmask_alloc(max_node_id + 1);
                if (MEMKIND_UNLIKELY((*nodes_mask)[i] == NULL)) {
                    ret = MEMKIND_ERROR_MALLOC;
                    log_err("numa_bitmask_alloc() failed.");
                    goto error;
                }

                copy_bitmask_to_bitmask(hi_cap_loc_mask, (*nodes_mask)[i]);
            }
        }
    }

    ret = MEMKIND_SUCCESS;
    goto success;

error:
    if (*nodes_mask) {
        for(i = 0; i < num_cpus; ++i) {
            if ((*nodes_mask)[i]) {
                numa_bitmask_free((*nodes_mask)[i]);
            }
        }
        free(*nodes_mask);
    }

success:
    numa_free_nodemask(hi_cap_loc_mask);
    numa_bitmask_free(node_cpus);
    free(local_nodes);
    hwloc_topology_destroy(topology);

    return ret;
}

int get_mem_attributes_hbw_nodes_mask(struct bitmask **hbw_node_mask)
{
    const char *hbw_threshold_env = memkind_get_env("MEMKIND_HBW_THRESHOLD");
    size_t hbw_threshold;
    int err;
    hwloc_topology_t topology;
    hwloc_obj_t init_node = NULL;

    // NUMA Nodes could be not in arithmetic progression
    int nodes_num = numa_max_node() + 1;
    *hbw_node_mask = numa_bitmask_alloc(nodes_num);
    if (*hbw_node_mask == NULL) {
        log_err("numa_bitmask_alloc() failed.");
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    if (hbw_threshold_env) {
        log_info("Environment variable MEMKIND_HBW_THRESHOLD detected: %s.",
                 hbw_threshold_env);
        int ret = sscanf(hbw_threshold_env, "%zud", &hbw_threshold);
        if (ret == 0) {
            log_err("Environment variable MEMKIND_HBW_THRESHOLD is invalid value.");
            numa_bitmask_free(*hbw_node_mask);
            return MEMKIND_ERROR_ENVIRON;
        }
    } else {
        hbw_threshold = MEMKIND_HBW_THRESHOLD_DEFAULT;
    }

    err = hwloc_topology_init(&topology);
    if (err) {
        numa_bitmask_free(*hbw_node_mask);
        log_err("hwloc initialize failed");
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    err = hwloc_topology_load(topology);
    if (err) {
        numa_bitmask_free(*hbw_node_mask);
        log_err("hwloc topology load failed");
        hwloc_topology_destroy(topology);
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    struct bitmask *node_cpus = numa_allocate_cpumask();
    while ((init_node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                   init_node)) != NULL) {
        struct hwloc_location initiator;
        hwloc_obj_t target = NULL;

        numa_node_to_cpus(init_node->os_index, node_cpus);
        if (numa_bitmask_weight(node_cpus) == 0) {
            log_info("Node skipped %d - no CPU detected in initiator Node.",
                     init_node->os_index);
            continue;
        }

        initiator.type = HWLOC_LOCATION_TYPE_CPUSET;
        initiator.location.cpuset = init_node->cpuset;

        while ((target = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                    target)) != NULL) {
            hwloc_uint64_t bandwidth;
            err = hwloc_memattr_get_value(topology, HWLOC_MEMATTR_ID_BANDWIDTH, target,
                                          &initiator, 0, &bandwidth);
            if (err) {
                log_info("Node skipped - cannot read initiator Node %d and target Node %d.",
                         init_node->os_index, target->os_index);
                continue;
            }

            if (bandwidth >= hbw_threshold) {
                numa_bitmask_setbit(*hbw_node_mask, target->os_index);
            }
        }
    }

    numa_bitmask_free(node_cpus);
    hwloc_topology_destroy(topology);
    if (numa_bitmask_weight(*hbw_node_mask) == 0) {
        numa_bitmask_free(*hbw_node_mask);
        return MEMKIND_ERROR_UNAVAILABLE;
    }
    return MEMKIND_SUCCESS;
}
#else
int get_per_cpu_hi_cap_local_nodes_mask(struct bitmask ***nodes_mask,
                                        int node_variant)
{
    log_err("Highest Local Capacity NUMA nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
}

int get_mem_attributes_hbw_nodes_mask(struct bitmask **hbw_node_mask)
{
    log_err("High Bandwidth NUMA nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
}
#endif
