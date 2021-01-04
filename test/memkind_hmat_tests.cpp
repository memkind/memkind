// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#include <memkind.h>
#include <numa.h>
#include <numaif.h>
#include "common.h"
#include "memory_topology.h"
#include <memory>
#include <unordered_map>

using MemoryTpg = std::pair<std::string,std::unique_ptr<AbstractTopology>>;
using MapMemoryTpg =
    std::unordered_map<MemoryTpg::first_type, MemoryTpg::second_type>;

static MapMemoryTpg TopologyMap;

class MemkindHMATFunctionalTestsParam: public ::testing::Test,
    public ::testing::WithParamInterface<memkind_t>
{
protected:
    memkind_t memory_kind;

    void SetUp()
    {
        memory_kind = GetParam();
        // TODO try use Setup for whole Suite
        if (TopologyMap.size() == 0) {
            //TODO find better way to lookup for number of classes
            TopologyMap.reserve(5);
            TopologyMap.emplace(MemoryTpg("KnightsMillAll2All", new KNM_All2All()));
            TopologyMap.emplace(MemoryTpg("KnightsMillSNC2", new KNM_SNC2()));
            TopologyMap.emplace(MemoryTpg("KnightsMillSNC4", new KNM_SNC4()));
            TopologyMap.emplace(MemoryTpg("CascadeLake2Var1", new CLX_2_var1()));
            TopologyMap.emplace(MemoryTpg("CascadeLake4Var1", new CLX_4_var1()));
        }
    }

    void TearDown()
    {}
};

INSTANTIATE_TEST_CASE_P(
    KindParam, MemkindHMATFunctionalTestsParam,
    ::testing::Values(MEMKIND_HBW));

static void *force_realloc_new_addr(memkind_t memory_kind, size_t size)
{
    void *init_ptr = memkind_malloc(memory_kind, 1);
    void *pr = memkind_realloc(memory_kind, init_ptr, 4099);
    void *new_ptr = memkind_malloc(memory_kind, size);
    memkind_free(memory_kind, pr);
    return new_ptr;
}

TEST_P(MemkindHMATFunctionalTestsParam,
       test_tc_memkind_HMAT_verify_InitTargetNode)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);

    const size_t size = 51*MB;
    int cpu_max = numa_num_configured_cpus();
    const char *env_p = std::getenv("MEMKIND_TEST_TOPOLOGY");
    auto topology = std::move(TopologyMap.at(env_p));

    std::vector<void *>addr_to_free;
    addr_to_free.reserve(cpu_max);

    for(int cpu_id=0; cpu_id<cpu_max; ++cpu_id) {
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(cpu_id, &cpu_set);
        status = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
        int cpu = sched_getcpu();
        ASSERT_EQ(cpu_id, cpu);
        void *ptr = force_realloc_new_addr(memory_kind, size);
        if (topology->is_kind_supported(memory_kind)) {
            ASSERT_NE(nullptr, ptr);
            memset(ptr, 0, size);
            int init_node = numa_node_of_cpu(cpu);
            ASSERT_NE(init_node, -1);
            int target_node = -1;
            status = get_mempolicy(&target_node, nullptr, 0, ptr,
                                   MPOL_F_NODE | MPOL_F_ADDR);
            ASSERT_EQ(status, 0);
            auto res = topology->verify_kind(memory_kind, {init_node, target_node});
            ASSERT_EQ(true, res);
            addr_to_free.push_back(ptr);
        } else {
            ASSERT_EQ(nullptr, ptr);
        }
    }
    for(auto const &val: addr_to_free) {
        memkind_free(memory_kind, val);
    }
}
