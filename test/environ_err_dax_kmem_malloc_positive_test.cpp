// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include "memkind/internal/memkind_dax_kmem.h"
#include <memkind.h>

#include <numa.h>
#include <numaif.h>

#include "common.h"

#include <climits>
#include <vector>

/* This test is run with overridden MEMKIND_DAX_KMEM_NODES environment variable
 * and tries to perform allocation on persistent memory using memkind_malloc()
 */
int main()
{
    struct bitmask *all_dax_kmem_nodes_nodemask = nullptr;
    void *ptr = nullptr;
    int ret = 1;
    std::vector<int> closest_numa_ids;
    int expected_numa_id = -1;
    int returned_numa_id = -1;
    char *env_value_str = getenv("MEMKIND_DAX_KMEM_NODES");
    int process_cpu = sched_getcpu();
    int process_node = numa_node_of_cpu(process_cpu);
    unsigned numa_max_id = (unsigned)numa_max_node();
    int min_distance;

    if (!env_value_str) {
        printf("Error: MEMKIND_DAX_KMEM environment variable is empty.\n");
        goto exit;
    }

    ptr = memkind_malloc(MEMKIND_DAX_KMEM, KB);
    if (ptr == nullptr) {
        printf("Error: allocation failed\n");
        goto exit;
    }
    memset(ptr, 'A', KB / 2);

    all_dax_kmem_nodes_nodemask = numa_parse_nodestring(env_value_str);
    min_distance = INT_MAX;
    for (unsigned i = 0; i <= numa_max_id; ++i) {
        if (numa_bitmask_isbitset(all_dax_kmem_nodes_nodemask, i)) {
            int distance_to_i_node = numa_distance(process_node, i);
            if (distance_to_i_node < min_distance) {
                min_distance = distance_to_i_node;
                closest_numa_ids.clear();
                closest_numa_ids.push_back(i);
            } else if (distance_to_i_node == min_distance) {
                closest_numa_ids.push_back(i);
            }
        }
    }

    get_mempolicy(&returned_numa_id, nullptr, 0, ptr,
                  MPOL_F_NODE | MPOL_F_ADDR);

    for (auto const &closest_numa : closest_numa_ids) {
        if (closest_numa == returned_numa_id) {
            expected_numa_id = closest_numa;
            ret = 0;
            break;
        }
    }
    if (ret) {
        printf(
            "Error: Memkind dax kmem and allocated pointer node id's are not equal: %d!=%d\n",
            expected_numa_id, returned_numa_id);
    }

exit:
    if (all_dax_kmem_nodes_nodemask) {
        numa_free_nodemask(all_dax_kmem_nodes_nodemask);
    }
    if (ptr) {
        memkind_free(MEMKIND_DAX_KMEM, ptr);
    }

    return ret;
}
