// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <memkind/internal/memkind_mem_attributes.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>

#include "config.h"
#ifdef MEMKIND_HWLOC
#include <hwloc.h>
#define MEMKIND_HBW_THRESHOLD_DEFAULT (200 * 1024) // Default threshold is 200 GB/s
#endif

#ifdef MEMKIND_HWLOC
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
int get_mem_attributes_hbw_nodes_mask(struct bitmask **hbw_node_mask)
{
    log_err("High Bandwidth NUMA nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
}
#endif
