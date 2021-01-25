// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#include <memkind/internal/memkind_mem_attributes.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>

#include "config.h"

#ifdef MEMKIND_HWLOC
#include <hwloc.h>
#include <hwloc/linux-libnuma.h>
#define MEMKIND_HBW_THRESHOLD_DEFAULT (200 * 1024) // Default threshold is 200 GB/s

int get_per_cpu_local_nodes_mask(struct bitmask ***nodes_mask,
                                 memkind_node_variant_t node_variant, memory_attribute_t attr)
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
        err = hwloc_get_local_numanode_objs(topology, &initiator, &num_local_nodes,
                                            local_nodes, 0);
        if (err) {
            log_err("hwloc_get_local_numanode_objs");
            ret = MEMKIND_ERROR_UNAVAILABLE;
            goto error;
        }

        hwloc_bitmap_zero(attr_loc_mask);
        switch(attr) {
            case MEM_ATTR_CAPACITY:
                best_mem_attr = 0;
                for (i = 0; i < num_local_nodes; ++i) {
                    if (local_nodes[i]->attr->numanode.local_memory == 0) {
                        log_info("Node skipped - Node %d is without memory.", i);
                        continue;
                    }

                    if (local_nodes[i]->attr->numanode.local_memory > best_mem_attr) {
                        best_mem_attr = local_nodes[i]->attr->numanode.local_memory;
                        hwloc_bitmap_only(attr_loc_mask, local_nodes[i]->os_index);
                    } else if (local_nodes[i]->attr->numanode.local_memory == best_mem_attr) {
                        hwloc_bitmap_set(attr_loc_mask, local_nodes[i]->os_index);
                    }
                }
                break;

            case MEM_ATTR_BANDWIDTH:
                best_mem_attr = 0;
                for (i = 0; i < num_local_nodes; ++i) {
                    err = hwloc_memattr_get_value(topology, HWLOC_MEMATTR_ID_BANDWIDTH,
                                                  local_nodes[i],
                                                  &initiator, 0, &mem_attr);
                    if (err) {
                        log_info("Node skipped - cannot read initiator Node %d and target Node %d.",
                                 init_node->os_index, local_nodes[i]->os_index);
                        continue;
                    }

                    if (mem_attr > best_mem_attr) {
                        best_mem_attr = mem_attr;
                        hwloc_bitmap_only(attr_loc_mask, local_nodes[i]->os_index);
                    } else if (mem_attr == best_mem_attr) {
                        hwloc_bitmap_set(attr_loc_mask, local_nodes[i]->os_index);
                    }
                }
                break;

            case MEM_ATTR_LATENCY:
                best_mem_attr = INT_MAX;
                for (i = 0; i < num_local_nodes; ++i) {
                    err = hwloc_memattr_get_value(topology, HWLOC_MEMATTR_ID_LATENCY,
                                                  local_nodes[i],
                                                  &initiator, 0, &mem_attr);
                    if (err) {
                        log_info("Node skipped - cannot read initiator Node %d and target Node %d.",
                                 init_node->os_index, local_nodes[i]->os_index);
                        continue;
                    }

                    if (mem_attr < best_mem_attr) {
                        best_mem_attr = mem_attr;
                        hwloc_bitmap_only(attr_loc_mask, local_nodes[i]->os_index);
                    } else if (mem_attr == best_mem_attr) {
                        hwloc_bitmap_set(attr_loc_mask, local_nodes[i]->os_index);
                    }
                }
                break;

            default:
                log_err("Unknown memory attribute.");
                ret = MEMKIND_ERROR_UNAVAILABLE;
                goto error;
        }

        mem_attr_nodes = hwloc_bitmap_weight(attr_loc_mask);

        if (node_variant == NODE_VARIANT_SINGLE &&
            mem_attr_nodes > 1) {
            ret = MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
            log_err("Multiple NUMA Nodes have the same value of memory attribute for init node %d.",
                    init_node->os_index);
            goto error;
        }

        if (mem_attr_nodes == 0) {
            ret = MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
            log_err("No memory attribute Nodes for init node %d.", init_node->os_index);
            goto error;
        }

        // populate memory attribute nodemask to all CPU's from initiator NUMA node
        hwloc_bitmap_foreach_begin(i, init_node->cpuset)
        (*nodes_mask)[i] = hwloc_nodeset_to_linux_libnuma_bitmask(topology,
                                                                  attr_loc_mask);
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
    for(i = 0; i < num_cpus; ++i) {
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

int get_mem_attributes_hbw_nodes_mask(struct bitmask **hbw_node_mask)
{
    const char *hbw_threshold_env = memkind_get_env("MEMKIND_HBW_THRESHOLD");
    size_t hbw_threshold;
    int err;
    hwloc_topology_t topology;
    hwloc_obj_t init_node = NULL;
    hwloc_cpuset_t node_cpus = NULL;

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
    if (MEMKIND_UNLIKELY(err)) {
        numa_bitmask_free(*hbw_node_mask);
        log_err("hwloc initialize failed");
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    err = hwloc_topology_load(topology);
    if (MEMKIND_UNLIKELY(err)) {
        numa_bitmask_free(*hbw_node_mask);
        log_err("hwloc topology load failed");
        hwloc_topology_destroy(topology);
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    node_cpus = hwloc_bitmap_alloc();
    if (MEMKIND_UNLIKELY(node_cpus == NULL)) {
        numa_bitmask_free(*hbw_node_mask);
        log_err("hwloc_bitmap_alloc failed");
        hwloc_topology_destroy(topology);
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    while ((init_node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                   init_node)) != NULL) {
        struct hwloc_location initiator;
        hwloc_obj_t target = NULL;

        // skip this node if it doesn't contain any CPU
        if (hwloc_bitmap_isincluded(init_node->cpuset, node_cpus)) {
            log_info("Node %d skipped - no CPU detected in initiator Node.",
                     init_node->os_index);
            continue;
        }
        hwloc_bitmap_or(node_cpus, node_cpus, init_node->cpuset);

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

    hwloc_bitmap_free(node_cpus);
    hwloc_topology_destroy(topology);
    if (numa_bitmask_weight(*hbw_node_mask) == 0) {
        numa_bitmask_free(*hbw_node_mask);
        return MEMKIND_ERROR_UNAVAILABLE;
    }
    return MEMKIND_SUCCESS;
}
#else
int get_per_cpu_local_nodes_mask(struct bitmask ***nodes_mask,
                                 memkind_node_variant_t node_variant, memory_attribute_t attr)
{
    log_err("Memory attribute NUMA nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
}

int get_mem_attributes_hbw_nodes_mask(struct bitmask **hbw_node_mask)
{
    log_err("High Bandwidth NUMA nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
}
#endif
