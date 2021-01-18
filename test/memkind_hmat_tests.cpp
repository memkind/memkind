// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#include <memkind.h>
#include <memory>
#include <numa.h>
#include <numaif.h>
#include <omp.h>
#include <sys/sysinfo.h>
#include <unordered_map>

#include "common.h"
#include "memory_topology.h"
#include "proc_stat.h"

using TpgPtr = std::unique_ptr<AbstractTopology>;
using MemoryTpg = std::pair<std::string, TpgPtr>;
using MapMemoryTpg =
    std::unordered_map<MemoryTpg::first_type, MemoryTpg::second_type>;

static MapMemoryTpg TopologyMap;

class MemkindHMATFunctionalTestsParam: public ::testing::Test,
    public ::testing::WithParamInterface<memkind_t>
{
protected:
    memkind_t memory_kind;
    const char *memory_tpg;

    void SetUp()
    {
        memory_kind = GetParam();
        memory_tpg = std::getenv("MEMKIND_TEST_TOPOLOGY");
        if (memory_tpg == nullptr)
            GTEST_SKIP() << "This test requires MEMKIND_TEST_TOPOLOGY";
        // TODO try use Setup for whole Suite
        if (TopologyMap.size() == 0) {
            //TODO find better way to lookup for number of classes
            TopologyMap.reserve(11);
            TopologyMap.emplace(MemoryTpg("KnightsMillAll2All", TpgPtr(new KNM_All2All)));
            TopologyMap.emplace(MemoryTpg("KnightsMillSNC2", TpgPtr(new KNM_SNC2)));
            TopologyMap.emplace(MemoryTpg("KnightsMillSNC4", TpgPtr(new KNM_SNC4)));
            TopologyMap.emplace(MemoryTpg("CascadeLake2Var1", TpgPtr(new CLX_2_var1)));
            TopologyMap.emplace(MemoryTpg("CascadeLake2Var1HMAT",
                                          TpgPtr(new CLX_2_var1_hmat)));
            TopologyMap.emplace(MemoryTpg("CascadeLake2Var2", TpgPtr(new CLX_2_var2)));
            TopologyMap.emplace(MemoryTpg("CascadeLake2Var3", TpgPtr(new CLX_2_var3)));
            TopologyMap.emplace(MemoryTpg("CascadeLake4Var1", TpgPtr(new CLX_4_var1)));
            TopologyMap.emplace(MemoryTpg("CascadeLake4Var2", TpgPtr(new CLX_4_var2)));
            TopologyMap.emplace(MemoryTpg("CascadeLake4Var3", TpgPtr(new CLX_4_var3)));
            TopologyMap.emplace(MemoryTpg("CascadeLake4Var4", TpgPtr(new CLX_4_var4)));

            std::cout << "MEMKIND_TEST_TOPOLOGY is: " << memory_tpg << std::endl;
        }
    }

    void TearDown()
    {}
};

INSTANTIATE_TEST_CASE_P(
    KindParam, MemkindHMATFunctionalTestsParam,
    ::testing::Values(MEMKIND_HBW,
                      MEMKIND_HIGHEST_CAPACITY_LOCAL,
                      MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED));

TEST_P(MemkindHMATFunctionalTestsParam,
       test_tc_memkind_HMAT_verify_InitTargetNode)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    // use big size to ensure that we call jemalloc extent
    const size_t size = 11*MB-5;
    int threads_num = get_nprocs();
    auto &topology = TopologyMap.at(memory_tpg);
    #pragma omp parallel for num_threads(threads_num)
    for(int thread_id=0; thread_id<threads_num; ++thread_id) {
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(thread_id, &cpu_set);
        int status = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
        int cpu = sched_getcpu();
        EXPECT_EQ(thread_id, cpu);
        void *ptr = memkind_malloc(memory_kind, size);
        if (topology->is_kind_supported(memory_kind)) {
            EXPECT_TRUE(ptr != nullptr);
            memset(ptr, 0, size);
            int init_node = numa_node_of_cpu(cpu);
            EXPECT_NE(-1, init_node);
            int target_node = -1;
            status = get_mempolicy(&target_node, nullptr, 0, ptr,
                                   MPOL_F_NODE | MPOL_F_ADDR);
            EXPECT_EQ(0, status);
            auto res = topology->verify_nodes(memory_kind, {init_node, target_node});
            EXPECT_EQ(true, res);
            res = topology->verify_addr(memory_kind, ptr);
            EXPECT_EQ(true, res);
            memkind_free(memory_kind, ptr);
        } else {
            EXPECT_TRUE(ptr == nullptr);
        }
    }
}
