// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#include <memkind.h>
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
    ::testing::Values(MEMKIND_HBW,
                      MEMKIND_HIGHEST_CAPACITY_LOCAL,
                      MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED,
                      MEMKIND_LOWEST_LATENCY_LOCAL,
                      MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED,
                      MEMKIND_HIGHEST_BANDWIDTH_LOCAL,
                      MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED));

TEST_P(MemkindHMATFunctionalTestsParam,
       test_tc_memkind_HMAT_verify_InitTargetNode)
{
    if (tp.is_libhwloc_supported()) {
        int status = numa_available();
        ASSERT_EQ(status, 0);
        // use big size to ensure that we call jemalloc extent
        const size_t size = 11*MB-5;
        int threads_num = get_nprocs();
        auto topology = TopologyFactory(memory_tpg);
        #pragma omp parallel for num_threads(threads_num)
        for(int thread_id=0; thread_id<threads_num; ++thread_id) {
            cpu_set_t cpu_set;
            CPU_ZERO(&cpu_set);
            CPU_SET(thread_id, &cpu_set);
            status = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
            int cpu = sched_getcpu();
            EXPECT_EQ(thread_id, cpu);
            void *ptr = memkind_malloc(memory_kind, size);
            if (topology->is_kind_supported(memory_kind)) {
                EXPECT_TRUE(ptr != nullptr);
                memset(ptr, 0, size);
                int init_node = numa_node_of_cpu(cpu);
                auto res = topology->verify_kind(memory_kind, init_node, ptr);
                EXPECT_EQ(true, res);
                memkind_free(memory_kind, ptr);
            } else {
                EXPECT_TRUE(ptr == nullptr);
            }
        }
    } else {
        GTEST_SKIP() << "libhwloc is required." << std::endl;
    }
}

TEST_P(MemkindHMATFunctionalTestsParam, test_tc_memkind_HMAT_without_hwloc)
{
    if (!tp.is_libhwloc_supported()) {
        const size_t size = 512;
        void *ptr = memkind_malloc(memory_kind, size);
        if (tp.is_KN_family_supported() && memory_kind == MEMKIND_HBW) {
            EXPECT_TRUE(ptr != nullptr);
            memkind_free(MEMKIND_HBW, ptr);
        } else
            EXPECT_TRUE(ptr == nullptr);
    } else {
        GTEST_SKIP() << "Lack of libhwloc is required." << std::endl;
    }
}
