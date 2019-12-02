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

#include <memkind.h>

#include <climits>
#include <numa.h>
#include <unistd.h>
#include <vector>

#include "common.h"

class MemkindDaxKmemTests: public ::testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindDaxKmemTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_free_with_NULL_kind_4096_bytes)
{
    void *ptr = memkind_malloc(MEMKIND_DAX_KMEM, 4096);
    ASSERT_NE(nullptr, ptr);
    memkind_free(nullptr, ptr);
}


static size_t get_kmem_dax_nodes(std::vector<int>& dax_kmem_nodes) {
    struct bitmask *cpu_mask = numa_allocate_cpumask();
    long sum_of_kmem_dax_free_space = 0;
    long long free_space;

    const int MAXNODE_ID = numa_num_configured_nodes();
    for (int id = 0; id < MAXNODE_ID; ++id) {
        numa_node_to_cpus(id, cpu_mask);

        // Check if numa node exists and if it is KMEM DAX NUMA
        if (numa_node_size64(id, &free_space) > 0 && numa_bitmask_weight(cpu_mask) == 0) {
            dax_kmem_nodes.push_back(id);
            sum_of_kmem_dax_free_space += free_space;
        }
    }
    numa_free_cpumask(cpu_mask);

    return sum_of_kmem_dax_free_space;
}

static std::vector<int> get_closest_numa_node(std::vector<int> dax_kmem_nodes) {
    int min_distance = INT_MAX;
    int process_cpu = sched_getcpu();
    int process_node = numa_node_of_cpu(process_cpu);
    std::vector<int> closest_numa_ids;

    for (unsigned i = 0; i < dax_kmem_nodes.size(); ++i) {
        int distance_to_i_node = numa_distance(process_node, dax_kmem_nodes[i]);
        if (distance_to_i_node < min_distance) {
            min_distance = distance_to_i_node;
            closest_numa_ids.clear();
            closest_numa_ids.push_back(dax_kmem_nodes[i]);
        } else if (distance_to_i_node == min_distance) {
            closest_numa_ids.push_back(dax_kmem_nodes[i]);
        }
    }

    return closest_numa_ids;
}

static bool is_swap_used(size_t alloc_size_KB) {
    char command[128];
    char output[128];

    sprintf(command, "awk '/VmSwap/{print $2}' /proc/%d/status", getpid());
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        std::cerr << "Could not start pipe command." << std::endl;
        return false;
    }

    while (fgets(output, sizeof(output), pipe) != nullptr) {}
    pclose(pipe);

    size_t used_swap_KB = std::stoul(output);
    if (used_swap_KB > alloc_size_KB) return true;

    return false;
}
