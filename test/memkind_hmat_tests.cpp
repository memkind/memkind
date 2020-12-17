// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <memkind.h>
#include <numa.h>
#include <numaif.h>
#ifdef MEMKIND_HWLOC
#include <hwloc.h>
#endif

#include "common.h"
#include "config.h"
#include "proc_stat.h"

#ifdef MEMKIND_HWLOC

typedef std::map<int, std::vector<int>> NumaMap;
typedef std::pair<std::string, NumaMap> Topology;

// NOTE: this is hardcoded and should match topoligies in utils/qemu/topology
// This map shows what are EXPECTED NUMA nodes for LOCAL_HIGHEST_CAPACITY
// kind allocation made from given initiator node, e.g.
// {1, {2,3}} -> CPU(s) on node #1 should allocate from nodes #2 or #3
const std::vector<Topology> t = {
    {
        "KnightsMillSNC2", {
            {0, {0}},
            {1, {1}}
        }
    },
    {
        "KnightsMillSNC4", {
            {0, {0}},
            {1, {1}},
            {2, {2}},
            {3, {3}}
        }
    },
    {
        "KnightsMillAll2All", {
            {0, {0}}
        }
    },
    {
        "CascadeLake2Var1", {
            {0, {2}},
            {1, {1}}
        }
    },
    {
        "CascadeLake4Var1", {
            {0, {4}},
            {1, {1}},
            {2, {5}},
            {3, {6}}
        }
    }
    // TODO add cases with multiple LOCAL_HIGHEST_CAPACITY nodes
};

class MemkindHMATFunctionalTests:
    public ::testing::Test,
    public ::testing::WithParamInterface<Topology>
{
protected:
    Topology param;
    NumaMap numaMap;

    void SetUp()
    {
        const char *envp = std::getenv("MEMKIND_TEST_TOPOLOGY");
        if (envp == nullptr)
            GTEST_SKIP();

        printf("MEMKIND_TEST_TOPOLOGY: %s\n", envp);

        param = GetParam();
        const char *testTopologyName = param.first.c_str();
        if (strcmp(envp, testTopologyName) != 0)
            GTEST_SKIP() << testTopologyName;

        numaMap = param.second;
    }

    void TearDown() {}
};

INSTANTIATE_TEST_CASE_P(
    TopologyParam, MemkindHMATFunctionalTests,
    ::testing::ValuesIn(t));

TEST_P(MemkindHMATFunctionalTests, test_TC_LocalHiCapacity_correct_numa)
{
    memkind_t kind = MEMKIND_LOCAL_HIGHEST_CAPACITY;
    int size = 10;
    void *ptr = memkind_malloc(kind, size);
    ASSERT_NE(ptr, nullptr);
    memset(ptr, 0, size);

    int cpu = sched_getcpu();
    ASSERT_GE(cpu, -1);
    int cpu_node = numa_node_of_cpu(cpu);
    ASSERT_GE(cpu_node, -1);

    // get ID and capacity of NUMA node where the allocation is made
    int numa_id = -1;
    int ret = get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    ASSERT_EQ(ret, 0);

    // NumaMap contains pairs of (cpu_node, hi_cap_node_vec) where:
    // * cpu_node is a NUMA node with CPU that made an allocation and
    // * hi_cap_node_vec is a vector NUMA node(s) that has the highest capacity
    //   among nodes that lies on the same package (has the same
    //   HWLOC_OBJ_PACKAGE parent)
    ASSERT_NE(numaMap.find(cpu_node), numaMap.end());
    auto &hi_cap_node_vec = numaMap.find(cpu_node)->second;

    bool correct = false;
    for (auto &&node : hi_cap_node_vec) {
        if (numa_id == node) {
            correct = true;
            break;
        }
    }
    ASSERT_EQ(correct, true);

    memkind_free(MEMKIND_LOCAL_HIGHEST_CAPACITY, ptr);
}

TEST_P(MemkindHMATFunctionalTests,
       test_TC_LocalHiCapacity_alloc_until_full_numa)
{
    memkind_t kind = MEMKIND_LOCAL_HIGHEST_CAPACITY;

    ProcStat stat;
    void *ptr;
    const size_t alloc_size = 100 * MB;
    std::set<void *> allocations;

    int cpu = sched_getcpu();
    ASSERT_GE(cpu, -1);
    int cpu_node = numa_node_of_cpu(cpu);
    ASSERT_GE(cpu_node, -1);

    size_t sum_of_free_space = 0;
    ASSERT_NE(numaMap.find(cpu_node), numaMap.end());
    auto &hi_cap_node_vec = numaMap.find(cpu_node)->second;

    for (auto &&i : hi_cap_node_vec) {
        long long numa_free = 0;
        numa_node_size64(i, &numa_free);
        sum_of_free_space += numa_free;
    }

    int numa_id = -1;
    while (sum_of_free_space > alloc_size * allocations.size()) {
        ptr = memkind_malloc(kind, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);

        int ret = get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
        ASSERT_EQ(ret, 0);

        bool correct = false;
        for (auto &&node : hi_cap_node_vec) {
            if (numa_id == node) {
                correct = true;
                break;
            }
        }
        ASSERT_EQ(correct, true);
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
}

#endif // MEMKIND_HWLOC
