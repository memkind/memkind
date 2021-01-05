// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <stdio.h>
#include "config.h"

#ifdef MEMKIND_HWLOC
#include "hwloc.h"
#include "numa.h"

static struct bitmask *node_mask;

static void print_array(const int *array_to_print, const char *header)
{
    int num_nodes = numa_num_configured_nodes();
    int i = 0;
    int j = 0;
    printf("%s\n",header);
    printf("node");
    for (i=0; i<=node_mask->size; ++i) {
        if (numa_bitmask_isbitset(node_mask, i)) {
            printf("%6d ", i);
        }
    }
    printf("\n");
    int row = 0;
    for (i=0; i<=node_mask->size; ++i) {
        if (numa_bitmask_isbitset(node_mask, i)) {
            printf("%3d:", i);
            for (j=0; j<num_nodes; ++j) {
                int val = *(array_to_print + row*num_nodes + j);
                if (val == -1) {
                    printf("%6s ", "-");
                } else {
                    printf("%6d ", val);
                }
            }
            row++;
        }
        printf("\n");
    }
}

static int print_all(void)
{
    int num_nodes = numa_num_configured_nodes();
    int num_node_max_id = numa_max_node();
    int err;
    int i = 0;
    int j = 0;

    hwloc_topology_t topology;
    hwloc_obj_t init_node = NULL;
    err = numa_available();
    if (err == -1) {
        printf("NUMA not available\n");
        return -1;
    }
    err = hwloc_topology_init(&topology);
    if (err) {
        printf("hwloc initialization failed\n");
        return -1;
    }
    err = hwloc_topology_load(topology);
    if (err) {
        printf("hwloc topology load failed\n");
        hwloc_topology_destroy(topology);
        return -1;
    }
    node_mask = numa_bitmask_alloc(num_node_max_id+1);
    if (!node_mask) {
        printf("numa_bitmask_alloc failed\n");
        hwloc_topology_destroy(topology);
        return -1;
    }

    int *b_array = (int *)malloc(num_nodes * num_nodes * sizeof(int));
    int *l_array = (int *)malloc(num_nodes * num_nodes * sizeof(int));
    if (b_array == NULL) {
        printf("malloc b_array failed\n");
        goto free_numa_bitmask_alloc;
    }
    if (l_array == NULL) {
        printf("malloc l_array failed\n");
        goto free_bandwidth_array;
    }
    while ((init_node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                   init_node)) != NULL) {
        numa_bitmask_setbit(node_mask, init_node->os_index);
        hwloc_obj_t target_node = NULL;
        j = 0;
        while ((target_node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                         target_node)) != NULL) {
            hwloc_uint64_t mem_attr;
            struct hwloc_location initiator;
            initiator.type = HWLOC_LOCATION_TYPE_CPUSET;
            initiator.location.cpuset = init_node->cpuset;

            err = hwloc_memattr_get_value(topology, HWLOC_MEMATTR_ID_BANDWIDTH, target_node,
                                          &initiator, 0, &mem_attr);
            *(b_array + i*num_nodes + j) = (err) ? -1 : mem_attr;

            err = hwloc_memattr_get_value(topology, HWLOC_MEMATTR_ID_LATENCY, target_node,
                                          &initiator, 0, &mem_attr);
            *(l_array + i*num_nodes + j) = (err) ? -1 : mem_attr;

            j++;
        }
        i++;
    }
    print_array(b_array, "Node bandwidth [MiB/s]:");
    print_array(l_array, "Node latency [ns]:");

    free(l_array);

free_bandwidth_array:
    free(b_array);

free_numa_bitmask_alloc:
    numa_bitmask_free(node_mask);

    hwloc_topology_destroy(topology);
    return 0;
}
#else
static int print_all(void)
{
    printf("Libhwloc is not supported\n");
    return -1;
}
#endif

int main()
{
    return print_all();
}
