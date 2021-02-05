// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */
#include <sstream>
#include <vector>

#include "allocator_perf_tool/AllocatorFactory.hpp"
#include "allocator_perf_tool/Configuration.hpp"
#include "allocator_perf_tool/HugePageOrganizer.hpp"
#include "common.h"

// Test heap managers initialization performance.
class HeapManagerInitPerfTest: public ::testing::Test
{

protected:
    void SetUp()
    {
        // Calculate reference statistics.
        ref_time = allocator_factory
                       .initialize_allocator(AllocatorTypes::STANDARD_ALLOCATOR)
                       .total_time;
    }

    void TearDown()
    {}

    void run_test(unsigned allocator_type)
    {
        AllocatorFactory::initialization_stat stat =
            allocator_factory.initialize_allocator(
                AllocatorTypes::MEMKIND_DEFAULT);

        post_test(stat);
    }

    void post_test(AllocatorFactory::initialization_stat &stat)
    {
        // Calculate (%) distance to the reference time for function calls.
        stat.ref_delta_time =
            allocator_factory.calc_ref_delta(ref_time, stat.total_time);

        std::stringstream elapsed_time;
        elapsed_time << stat.total_time;

        std::stringstream ref_delta_time;
        ref_delta_time << std::fixed << stat.ref_delta_time << std::endl;

        RecordProperty("elapsed_time", elapsed_time.str());
        RecordProperty("ref_delta_time_percent_rate", ref_delta_time.str());

        for (int i = 0; i < stat.memory_overhead.size(); i++) {
            std::stringstream node;
            node << "memory_overhad_node_" << i;
            std::stringstream memory_overhead;
            memory_overhead << stat.memory_overhead[i];
            RecordProperty(node.str(), memory_overhead.str());
        }
    }

    AllocatorFactory allocator_factory;
    float ref_time;
};

TEST_F(HeapManagerInitPerfTest, test_TC_MEMKIND_perf_libinit_DEFAULT)
{
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(AllocatorTypes::MEMKIND_DEFAULT);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest, test_TC_MEMKIND_perf_libinit_HBW)
{
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(AllocatorTypes::MEMKIND_HBW);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest, test_TC_MEMKIND_perf_libinit_INTERLEAVE)
{
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(
            AllocatorTypes::MEMKIND_INTERLEAVE);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest, test_TC_MEMKIND_perf_libinit_HBW_INTERLEAVE)
{
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(
            AllocatorTypes::MEMKIND_HBW_INTERLEAVE);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest, test_TC_MEMKIND_perf_libinit_HBW_PREFERRED)
{
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(
            AllocatorTypes::MEMKIND_HBW_PREFERRED);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest, test_TC_MEMKIND_perf_libinit_HUGETLB)
{
    HugePageOrganizer huge_page_organizer(16);
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(AllocatorTypes::MEMKIND_HUGETLB);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest, test_TC_MEMKIND_perf_libinit_GBTLB)
{
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(AllocatorTypes::MEMKIND_GBTLB);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest, test_TC_MEMKIND_perf_libinit_HBW_HUGETLB)
{
    HugePageOrganizer huge_page_organizer(16);
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(
            AllocatorTypes::MEMKIND_HBW_HUGETLB);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest,
       test_TC_MEMKIND_perf_libinit_HBW_PREFERRED_HUGETLB)
{
    HugePageOrganizer huge_page_organizer(16);
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(
            AllocatorTypes::MEMKIND_HBW_PREFERRED_HUGETLB);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest, test_TC_MEMKIND_perf_ext_libinit_HBW_GBTLB)
{
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(
            AllocatorTypes::MEMKIND_HBW_GBTLB);
    post_test(stat);
}

TEST_F(HeapManagerInitPerfTest,
       test_TC_MEMKIND_perf_libinit_HBW_PREFERRED_GBTLB)
{
    AllocatorFactory::initialization_stat stat =
        allocator_factory.initialize_allocator(
            AllocatorTypes::MEMKIND_HBW_PREFERRED_GBTLB);
    post_test(stat);
}
