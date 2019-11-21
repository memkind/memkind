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
#include <set>
#include <vector>

#include "common.h"

#define GTEST_COUT std::cerr << "[          ] [ INFO ]"

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

TEST_F(MemkindDaxKmemTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_until_full_numa)
{
    void *ptr;
    long long free_space;
    const size_t alloc_size = 1 * MB;
    size_t numa_size;
    int numa_id = -1;
    std::vector<void *> allocations;

    ptr = memkind_malloc(MEMKIND_DAX_KMEM, alloc_size);
    ASSERT_TRUE(nullptr != ptr);
    memset(ptr, 'a', alloc_size);
    allocations.push_back(ptr);

    get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    numa_size = numa_node_size64(numa_id, nullptr);

    while (0.85 * numa_size > alloc_size * allocations.size()) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM, alloc_size);
        memset(ptr, 'a', alloc_size);
        numa_node_size64(numa_id, &free_space);
        allocations.push_back(ptr);
    }

    EXPECT_GE(10 * MB + alloc_size, unsigned(free_space));

    for (auto const &ptr: allocations) {
        memkind_free(MEMKIND_DAX_KMEM, ptr);
    }
}

TEST_F(MemkindDaxKmemTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_ALL_alloc_until_full_numa)
{
    void *ptr;
    const size_t alloc_size = 1 * MB;
    int numa_id = -1;
    long  sum_of_kmem_dax_free_space = 0;
    long long free_space;
    std::vector<void *> allocations;
    std::set<int> dax_kmem_nodes;
    struct bitmask *cpu_mask = numa_allocate_cpumask();

    const int MAXNODE_ID = numa_num_configured_nodes();
    for (int id = 0; id < MAXNODE_ID; ++id) {
        numa_node_to_cpus(id, cpu_mask);

        // Check if numa node exists and if it is KMEM DAX NUMA
        if (numa_node_size64(id, &free_space) > 0
        && numa_bitmask_weight(cpu_mask) == 0) {
            dax_kmem_nodes.insert(id);
            sum_of_kmem_dax_free_space += free_space;
        }
    }
    numa_free_cpumask(cpu_mask);

    while (0.85 * sum_of_kmem_dax_free_space > alloc_size * allocations.size()) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM_ALL, alloc_size);
        ASSERT_TRUE(nullptr != ptr);
        memset(ptr, 'a', alloc_size);

        get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
        ASSERT_EQ(size_t(1), dax_kmem_nodes.count(numa_id));

        allocations.push_back(ptr);
    }

    for (auto const &ptr: allocations) {
        memkind_free(MEMKIND_DAX_KMEM_ALL, ptr);
    }
}
