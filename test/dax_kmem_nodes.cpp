// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <climits>
#include <numa.h>
#include <numaif.h>

#include "dax_kmem_nodes.h"

DaxKmem::DaxKmem()
{
    dax_kmem_nodes = get_dax_kmem_nodes();
}

size_t DaxKmem::get_free_space(void)
{
    size_t sum_of_dax_kmem_free_space = 0;
    long long free_space;

    for(auto const &node: dax_kmem_nodes) {
        numa_node_size64(node, &free_space);
        sum_of_dax_kmem_free_space += free_space;
    }

    return sum_of_dax_kmem_free_space;
}

std::unordered_set<int> DaxKmem::get_closest_numa_nodes(int regular_node)
{
    int min_distance = INT_MAX;
    std::unordered_set<int> closest_numa_ids;

    for (auto const &node: dax_kmem_nodes) {
        int distance_to_i_node = numa_distance(regular_node, node);

        if (distance_to_i_node < min_distance) {
            min_distance = distance_to_i_node;
            closest_numa_ids.clear();
            closest_numa_ids.insert(node);
        } else if (distance_to_i_node == min_distance) {
            closest_numa_ids.insert(node);
        }
    }

    return closest_numa_ids;
}

std::unordered_set<int> DaxKmem::get_dax_kmem_nodes(void)
{
    struct bitmask *cpu_mask = numa_allocate_cpumask();
    std::unordered_set<int> dax_kmem_nodes;

    const int MAXNODE_ID = numa_max_node();
    for (int id = 0; id <= MAXNODE_ID; ++id) {
        numa_node_to_cpus(id, cpu_mask);

        // Check if NUMA node exists and if it is NUMA node created from persistent memory
        if (numa_node_size64(id, nullptr) > 0 &&
            numa_bitmask_weight(cpu_mask) == 0) {
            dax_kmem_nodes.insert(id);
        }
    }
    numa_free_cpumask(cpu_mask);

    return dax_kmem_nodes;
}

size_t DaxKmem::size(void)
{
    return dax_kmem_nodes.size();
}

bool DaxKmem::contains(int node)
{
    return (dax_kmem_nodes.find(node) != dax_kmem_nodes.end());
}
