// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <memkind.h>
#include <numa.h>
#include <numaif.h>
#include <string>
#include <unistd.h>
#include <math.h>
#include <numa.h>
#include <math.h>

#include "proc_stat.h"
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "TestPolicy.hpp"

#include "common.h"

class MemkindHiCapacityFunctionalTests:
    public ::testing::Test
{
protected:

    void SetUp() {}
    void TearDown() {}
};

TEST_F(MemkindHiCapacityFunctionalTests,
       test_TC_HiCapacity_alloc_size_max)
{
    errno = 0;
    void *test1 = memkind_malloc(MEMKIND_HIGHEST_CAPACITY, SIZE_MAX);
    ASSERT_EQ(test1, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_F(MemkindHiCapacityFunctionalTests, test_TC_HiCapacity_correct_numa)
{
    memkind_t kind = MEMKIND_HIGHEST_CAPACITY;
    int size = 10;
    void *ptr = memkind_malloc(kind, size);
    ASSERT_NE(ptr, nullptr);
    memset(ptr, 0, size);

    // get ID and capacity of NUMA node where the allocation is made
    int numa_id = -1;
    int ret = get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    ASSERT_EQ(ret, 0);
    long long numa_capacity = numa_node_size64(numa_id, NULL);

    // get capacity of NUMA node that has the highest capacity in the system
    int num_nodes = numa_num_configured_nodes();
    long long max_capacity = 0;
    for (int i = 0; i < num_nodes; ++i) {
        max_capacity = std::max(max_capacity, numa_node_size64(i, NULL));
    }

    ASSERT_EQ(numa_capacity, max_capacity);

    memkind_free(MEMKIND_HIGHEST_CAPACITY, ptr);
}

TEST_F(MemkindHiCapacityFunctionalTests, test_TC_HiCapacity_alloc_until_full_numa)
{
    memkind_t kind = MEMKIND_HIGHEST_CAPACITY;

    ProcStat stat;
    void *ptr;
    const size_t alloc_size = 100 * MB;
    std::set<void *> allocations;

    // get highest NUMA node capacity in the system
    int num_nodes = numa_num_configured_nodes();
    long long max_capacity = 0;
    for (int i = 0; i < num_nodes; ++i) {
        long long cur_capacity = numa_node_size64(i, NULL);
        if (cur_capacity > max_capacity) {
            max_capacity = cur_capacity;
        }
    }

    // create a bitmask with nodes that has highest capacity
    // and sum their free space
    struct bitmask *highest_nodes = numa_bitmask_alloc(num_nodes);
    size_t sum_of_free_space = 0;
    for (int i = 0; i < num_nodes; ++i) {
        long long numa_free;
        long long cur_capacity = numa_node_size64(i, &numa_free);
        if (cur_capacity == max_capacity) {
            numa_bitmask_setbit(highest_nodes, i);
            sum_of_free_space += numa_free;
        }
    }

    int numa_id = -1;
    while (sum_of_free_space > alloc_size * allocations.size()) {
        ptr = memkind_malloc(kind, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);

        int ret = get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(numa_bitmask_isbitset(highest_nodes, numa_id), 1);
    }

    const int n_swap_alloc = 5;
    size_t init_swap = stat.get_used_swap_space_size_bytes();
    for(int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(kind, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);
    }

    ASSERT_GE(stat.get_used_swap_space_size_bytes(), init_swap);

    for (auto const &ptr: allocations) {
        memkind_free(kind, ptr);
    }

    numa_bitmask_free(highest_nodes);
}
