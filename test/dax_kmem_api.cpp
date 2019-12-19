/*
* Copyright (C) 2019 Intel Corporation.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright notice(s),
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice(s),
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
* EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <climits>
#include "dax_kmem_api.h"

DaxKmem::DaxKmem()
{
    dax_kmem_nodes = get_dax_kmem_nodes();
}

size_t DaxKmem::get_free_dax_kmem_space(void)
{
    size_t sum_of_dax_kmem_free_space = 0;
    long long free_space;
    std::set<int> dax_kmem_nodes = get_dax_kmem_nodes();

    for(auto const &node: dax_kmem_nodes) {
        numa_node_size64(node, &free_space);
        sum_of_dax_kmem_free_space += free_space;
    }

    return sum_of_dax_kmem_free_space;
}

std::set<int> DaxKmem::get_closest_dax_kmem_numa_nodes(int regular_node)
{
    int min_distance = INT_MAX;
    std::set<int> closest_numa_ids;
    std::set<int> dax_kmem_nodes = get_dax_kmem_nodes();

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

std::set<int> DaxKmem::get_dax_kmem_nodes(void)
{
    struct bitmask *cpu_mask = numa_allocate_cpumask();
    std::set<int> dax_kmem_nodes;

    const int MAXNODE_ID = numa_max_node();
    for (int id = 0; id <= MAXNODE_ID; ++id) {
        numa_node_to_cpus(id, cpu_mask);

        // Check if numa node exists and if it is NUMA node created from persistent memory
        if (numa_node_size64(id, NULL) > 0 &&
            numa_bitmask_weight(cpu_mask) == 0) {
            dax_kmem_nodes.insert(id);
        }
    }
    numa_free_cpumask(cpu_mask);

    return dax_kmem_nodes;
}
