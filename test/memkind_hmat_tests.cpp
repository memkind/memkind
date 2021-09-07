// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#include <memkind.h>
#include <memkind/internal/memkind_private.h>
#include <memory>
#include <numa.h>
#include <omp.h>
#include <sys/sysinfo.h>

#include "TestPrereq.hpp"
#include "common.h"
#include "memory_topology.h"
#include "proc_stat.h"

class MemkindHMATFunctionalTestsParam: public ::Memkind_Param_Test
{
protected:
    const char *memory_tpg;
    TestPrereq tp;
    int threads_num = get_nprocs();

    void SetUp()
    {
        memory_tpg = std::getenv("MEMKIND_TEST_TOPOLOGY");
        std::cout << "MEMKIND_TEST_TOPOLOGY is: " << memory_tpg << std::endl;
        Memkind_Param_Test::SetUp();
    }

    void TearDown()
    {}
};

INSTANTIATE_TEST_CASE_P(
    KindParam, MemkindHMATFunctionalTestsParam,
    ::testing::Values(
        MEMKIND_HBW, MEMKIND_HBW_ALL, MEMKIND_HIGHEST_CAPACITY_LOCAL,
        MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED, MEMKIND_LOWEST_LATENCY_LOCAL,
        MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED, MEMKIND_HIGHEST_BANDWIDTH_LOCAL,
        MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED));

TEST_P(MemkindHMATFunctionalTestsParam,
       test_tc_memkind_HMAT_verify_InitTargetNode)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    // use big size to ensure that we call jemalloc extent
    const size_t size = 11 * MB - 5;
    auto topology = TopologyFactory(memory_tpg, memory_kind);
#pragma omp parallel for num_threads(threads_num)
    for (int thread_id = 0; thread_id < threads_num; ++thread_id) {
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(thread_id, &cpu_set);
        status = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
        int cpu = sched_getcpu();
        EXPECT_EQ(thread_id, cpu);
        void *ptr = topology->allocate(size);
        if (topology->is_kind_supported()) {
            EXPECT_TRUE(ptr != nullptr);
            memset(ptr, 0, size);
            int init_node = numa_node_of_cpu(cpu);
            auto res = topology->verify_kind(init_node, ptr);
            EXPECT_EQ(true, res);
            topology->deallocate(ptr);
        } else {
            EXPECT_TRUE(ptr == nullptr);
        }
    }
}

class MemkindHMATGetCapacityTestsParam: public ::MemkindHMATFunctionalTestsParam
{
};

INSTANTIATE_TEST_CASE_P(KindParam, MemkindHMATGetCapacityTestsParam,
                        ::testing::Values(MEMKIND_HBW, MEMKIND_HBW_ALL,
                                          MEMKIND_HBW_INTERLEAVE,
                                          MEMKIND_HIGHEST_CAPACITY_LOCAL,
                                          MEMKIND_LOWEST_LATENCY_LOCAL,
                                          MEMKIND_HIGHEST_BANDWIDTH_LOCAL));

TEST_P(MemkindHMATGetCapacityTestsParam,
       test_tc_memkind_HMAT_verify_get_capacity)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    auto topology = TopologyFactory(memory_tpg, memory_kind);
    if (!topology->is_kind_supported()) {
        GTEST_SKIP() << "Kind " << memory_kind->name
                     << " is unsupported with topology " << memory_tpg << " ."
                     << std::endl;
    }

#pragma omp parallel for num_threads(threads_num)
    for (int thread_id = 0; thread_id < threads_num; ++thread_id) {
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(thread_id, &cpu_set);
        status = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
        int cpu = sched_getcpu();
        EXPECT_EQ(thread_id, cpu);

        int init_node = numa_node_of_cpu(cpu);
        auto numa_nodes = topology->get_target_nodes(init_node);

        ssize_t capacity = tp.get_capacity(numa_nodes);
        EXPECT_GT(capacity, 0);

        EXPECT_EQ(capacity, memkind_get_capacity(memory_kind));
    }
}
