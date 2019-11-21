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
#include <numa.h>
#include <numaif.h>

#include <algorithm>
#include <climits>
#include <string>
#include <unistd.h>
#include <vector>

#include "common.h"
#include "sys/types.h"
#include "sys/sysinfo.h"

class MemkindDaxKmemTests: public ::testing::Test
{
protected:
    void *ptr;
    const size_t alloc_size = 100 * MB;
    std::vector<void *> allocations;
    std::vector<int> kmem_dax_nodes;
    size_t sum_of_kmem_dax_free_space = 0;
    const int n_swap_alloc = 20;

    void SetUp()
    {
        sum_of_kmem_dax_free_space = get_kmem_dax_nodes(kmem_dax_nodes);
        if (kmem_dax_nodes.size() < 1) {
            GTEST_SKIP() << "Minimum 1 kmem dax node required." << std::endl;
        }
    }

    void TearDown()
    {}

    size_t get_kmem_dax_nodes(std::vector<int>&);
    bool is_swap_used(size_t);
    std::vector<int> get_closest_numa_node(std::vector<int>);
};

TEST_F(MemkindDaxKmemTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_free_with_NULL_kind_4096_bytes)
{
    void *ptr = memkind_malloc(MEMKIND_DAX_KMEM, 4096);
    ASSERT_NE(nullptr, ptr);
    memkind_free(nullptr, ptr);
}

TEST_F(MemkindDaxKmemTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_until_full_numa)
{
    size_t numa_size;
    int numa_id = -1;

    ptr = memkind_malloc(MEMKIND_DAX_KMEM, alloc_size);
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', alloc_size);
    allocations.push_back(ptr);

    get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    numa_size = numa_node_size64(numa_id, nullptr);

    while (numa_size > alloc_size * allocations.size()) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM, alloc_size);
        memset(ptr, 'a', alloc_size);
        allocations.push_back(ptr);
    }

    for(int i = 0; i < 20; ++i) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
    }

    ASSERT_TRUE(is_swap_used(alloc_size / KB));

    for (auto const &ptr: allocations) {
        memkind_free(MEMKIND_DAX_KMEM, ptr);
    }
}

TEST_F(MemkindDaxKmemTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_ALL_alloc_until_full_numa)
{
    if (kmem_dax_nodes.size() < 2)
        GTEST_SKIP() << "This tests requires minimum 2 kmem dax nodes";

    int numa_id = -1;
    long sum_of_kmem_dax_free_space = 0;
    std::vector<int> kmem_dax_nodes;

    while (1.0 * sum_of_kmem_dax_free_space > alloc_size * allocations.size()) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM_ALL, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);

        get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
        ASSERT_NE(kmem_dax_nodes.end(), std::find(kmem_dax_nodes.begin(), kmem_dax_nodes.end(), numa_id));

        allocations.push_back(ptr);
    }

    for(int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM_ALL, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);

        allocations.push_back(ptr);
    }

    ASSERT_TRUE(is_swap_used(alloc_size / KB));

    for (auto const &ptr: allocations) {
        memkind_free(MEMKIND_DAX_KMEM_ALL, ptr);
    }
}

TEST_F(MemkindDaxKmemTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_PREFFERED_alloc_until_full_numa)
{
    size_t numa_size;
    int numa_id = -1;

    ptr = memkind_malloc(MEMKIND_DAX_KMEM, alloc_size);
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', alloc_size);
    allocations.push_back(ptr);

    get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    std::vector<int> closest_numa_ids = get_closest_numa_node(kmem_dax_nodes);
    numa_size = numa_node_size64(numa_id, nullptr);

    while (numa_size > alloc_size * allocations.size()) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM_PREFERRED, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);

        ASSERT_TRUE(closest_numa_ids.end() != std::find(closest_numa_ids.begin(), closest_numa_ids.end(), numa_id));

        allocations.push_back(ptr);
    }

    for(int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM_PREFERRED, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);

        allocations.push_back(ptr);
    }

    ASSERT_FALSE(is_swap_used(alloc_size / KB));

    for (auto const &ptr: allocations) {
        memkind_free(MEMKIND_DAX_KMEM_ALL, ptr);
    }
}

size_t MemkindDaxKmemTests::get_kmem_dax_nodes(std::vector<int>& dax_kmem_nodes) {
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

std::vector<int> MemkindDaxKmemTests::get_closest_numa_node(std::vector<int> dax_kmem_nodes) {
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
bool MemkindDaxKmemTests::is_swap_used(size_t alloc_size_KB) {
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
