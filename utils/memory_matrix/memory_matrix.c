// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "config.h"
#include <stdio.h>

#ifdef MEMKIND_HWLOC
#include "hwloc.h"
#include "numa.h"

static struct bitmask *node_cpumask;
static hwloc_topology_t topology;

static void print_attr(hwloc_memattr_id_t mem_attr)
{
    int max_node_id = numa_max_node();
    int init_id, target_id;

    if (mem_attr == HWLOC_MEMATTR_ID_BANDWIDTH) {
        printf("Node bandwidth [MiB/s]:\n");
    } else if (mem_attr == HWLOC_MEMATTR_ID_LATENCY) {
        printf("Node latency [ns]:\n");
    } else {
        printf("[ERROR] Unknown memory attribute\n");
        numa_free_cpumask(node_cpumask);
        hwloc_topology_destroy(topology);
        exit(-1);
    }

    printf("node");
    for (init_id = 0; init_id <= max_node_id; ++init_id) {
        if (numa_bitmask_isbitset(numa_all_nodes_ptr, init_id)) {
            printf("%6d ", init_id);
        }
    }
    printf("\n");
    for (init_id = 0; init_id <= max_node_id; ++init_id) {
        if (numa_bitmask_isbitset(numa_all_nodes_ptr, init_id)) {
            printf("%3d:", init_id);
            if (numa_node_to_cpus(init_id, node_cpumask)) {
                printf("[ERROR] numa_node_to_cpus\n");
                numa_free_cpumask(node_cpumask);
                hwloc_topology_destroy(topology);
                exit(-1);
            }
            if (numa_bitmask_weight(node_cpumask) == 0) {
                for (target_id = 0; target_id <= max_node_id; ++target_id) {
                    if (numa_bitmask_isbitset(numa_all_nodes_ptr, target_id)) {
                        printf("%6s ", "-");
                    }
                }
            } else {
                hwloc_uint64_t attr_val;
                struct hwloc_location initiator;
                hwloc_obj_t init_node =
                    hwloc_get_numanode_obj_by_os_index(topology, init_id);
                initiator.type = HWLOC_LOCATION_TYPE_CPUSET;
                initiator.location.cpuset = init_node->cpuset;
                for (target_id = 0; target_id <= max_node_id; ++target_id) {
                    if (numa_bitmask_isbitset(numa_all_nodes_ptr, target_id)) {
                        hwloc_obj_t target_node =
                            hwloc_get_numanode_obj_by_os_index(topology,
                                                               target_id);
                        int err = hwloc_memattr_get_value(
                            topology, mem_attr, target_node, &initiator, 0,
                            &attr_val);
                        if (err) {
                            printf("%6s ", "X");
                        } else {
                            printf("%6lu ", attr_val);
                        }
                    }
                }
            }
            printf("\n");
        }
    }
}

static int print_all(void)
{
    int err = numa_available();
    if (err == -1) {
        printf("[ERROR] NUMA not available\n");
        return -1;
    }
    err = hwloc_topology_init(&topology);
    if (err) {
        printf("[ERROR] hwloc initialization failed\n");
        return -1;
    }
    err = hwloc_topology_load(topology);
    if (err) {
        printf("[ERROR] hwloc topology load failed\n");
        hwloc_topology_destroy(topology);
        return -1;
    }
    node_cpumask = numa_allocate_cpumask();
    if (!node_cpumask) {
        printf("[ERROR] numa_allocate_cpumask\n");
        hwloc_topology_destroy(topology);
        return -1;
    }

    print_attr(HWLOC_MEMATTR_ID_BANDWIDTH);
    print_attr(HWLOC_MEMATTR_ID_LATENCY);

    numa_free_cpumask(node_cpumask);

    hwloc_topology_destroy(topology);
    return 0;
}
#else
static int print_all(void)
{
    printf("[ERROR] Libhwloc is not supported\n");
    return -1;
}
#endif

int main()
{
    return print_all();
}
