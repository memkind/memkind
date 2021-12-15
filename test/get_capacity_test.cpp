// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind.h>
#include <memkind/internal/memkind_private.h>
#include <sys/mman.h>

#include "TestPrereq.hpp"
#include "common.h"

class MemkindGetCapacityTests: public ::testing::Test
{
protected:
    TestPrereq tp;
};

class MemkindGetCapacityTestsDaxKmem: public ::testing::Test
{
protected:
    TestPrereq tp;

    void SetUp()
    {
        if (!tp.is_DAX_KMEM_supported()) {
            GTEST_SKIP() << "DAX KMEM is required." << std::endl;
        }
    }
};

class MemkindGetCapacityTestsPreferredParam: public ::Memkind_Param_Test
{
protected:
    TestPrereq tp;
};

class MemkindGetCapacityTestsFixedKindParam
    : public ::testing::Test,
      public ::testing::WithParamInterface<size_t>
{
};

INSTANTIATE_TEST_CASE_P(MmappedTest, MemkindGetCapacityTestsFixedKindParam,
                        ::testing::Values(1024 * 1024 * 512, 0, 1024, 25, 87));

INSTANTIATE_TEST_CASE_P(
    KindParam, MemkindGetCapacityTestsPreferredParam,
    ::testing::Values(MEMKIND_HBW_PREFERRED, MEMKIND_HBW_PREFERRED_HUGETLB,
                      MEMKIND_HBW_PREFERRED_GBTLB, MEMKIND_DAX_KMEM_PREFERRED,
                      MEMKIND_HIGHEST_CAPACITY_PREFERRED,
                      MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED,
                      MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED,
                      MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED));

TEST_F(MemkindGetCapacityTests, test_tc_memkind_verify_capacity_fsdax)
{
    const char *dir = "/tmp";
    ssize_t max_size = 2 * MEMKIND_PMEM_MIN_SIZE;
    struct memkind *kind = NULL;
    memkind_create_pmem(dir, max_size, &kind);

    ASSERT_EQ(max_size, memkind_get_capacity(kind));

    max_size = 0;
    memkind_create_pmem(dir, max_size, &kind);
    ASSERT_GT(memkind_get_capacity(kind), 0);

    memkind_destroy_kind(kind);
}

TEST_P(MemkindGetCapacityTestsFixedKindParam,
       test_tc_memkind_verify_capacity_mmapped)
{
    const size_t MMAPPED_SIZE = GetParam();
    void *addr = mmap(NULL, MMAPPED_SIZE, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    struct memkind *kind = NULL;
    memkind_create_fixed(addr, MMAPPED_SIZE, &kind);
    ASSERT_EQ((const ssize_t)MMAPPED_SIZE, memkind_get_capacity(kind));
    memkind_destroy_kind(kind);
    munmap(addr, MMAPPED_SIZE);
}

TEST_F(MemkindGetCapacityTests, test_tc_memkind_verify_capacity_default)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    auto numa_nodes = tp.get_all_numa_nodes();
    ssize_t capacity = tp.get_capacity(numa_nodes);
    ASSERT_GT(capacity, 0);

    ASSERT_EQ(capacity, memkind_get_capacity(MEMKIND_DEFAULT));
}

TEST_F(MemkindGetCapacityTests, test_tc_memkind_verify_capacity_high_cap)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);

    ssize_t capacity = -1;
    ssize_t highest_capacity = -1;
    auto numa_nodes = tp.get_all_numa_nodes();
    for (auto node_id : numa_nodes) {
        long long node_capacity = numa_node_size64(node_id, NULL);
        if (node_capacity > highest_capacity) {
            highest_capacity = node_capacity;
            capacity = node_capacity;
        } else if (node_capacity == highest_capacity) {
            capacity += node_capacity;
        }
    }
    ASSERT_GT(capacity, 0);

    ASSERT_EQ(capacity, memkind_get_capacity(MEMKIND_HIGHEST_CAPACITY));
}

TEST_F(MemkindGetCapacityTests, test_tc_memkind_verify_capacity_hugetlb)
{
    size_t size = 1 * MB;
    memkind_t kind = MEMKIND_HUGETLB;
    void *ptr = memkind_malloc(kind, size);
    if (ptr == NULL) {
        GTEST_SKIP() << "Cannot allocate using " << kind->name
                     << " kind. Check if hugepages are available." << std::endl;
    }
    memkind_free(kind, ptr);

    ASSERT_GT(memkind_get_capacity(kind), 0);
}

TEST_F(MemkindGetCapacityTests, test_tc_memkind_verify_capacity_interleave)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    auto numa_nodes = tp.get_all_numa_nodes();
    ssize_t capacity = tp.get_capacity(numa_nodes);
    ASSERT_GT(capacity, 0);

    ASSERT_EQ(capacity, memkind_get_capacity(MEMKIND_INTERLEAVE));
}

TEST_F(MemkindGetCapacityTests, test_tc_memkind_verify_capacity_regular)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    auto numa_nodes = tp.get_regular_numa_nodes();
    ssize_t capacity = tp.get_capacity(numa_nodes);
    ASSERT_GT(capacity, 0);

    memkind_t kind = MEMKIND_REGULAR;
    void *ptr = memkind_malloc(kind, KB);
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', KB);
    ASSERT_EQ(capacity, memkind_get_capacity(kind));
    memkind_free(kind, ptr);
}

TEST_F(MemkindGetCapacityTestsDaxKmem, test_tc_memkind_verify_capacity_dax_kmem)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    memkind_t kind = MEMKIND_DAX_KMEM;

    int process_cpu = sched_getcpu();
    int process_node = numa_node_of_cpu(process_cpu);

    auto dax_kmem_nodes = tp.get_memory_only_numa_nodes();
    auto numa_nodes = tp.get_closest_numa_nodes(process_node, dax_kmem_nodes);
    ssize_t capacity = tp.get_capacity(numa_nodes);
    ASSERT_GT(capacity, 0);

    ASSERT_EQ(capacity, memkind_get_capacity(kind));
}

TEST_F(MemkindGetCapacityTestsDaxKmem,
       test_tc_memkind_verify_capacity_dax_kmem_all)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    memkind_t kind = MEMKIND_DAX_KMEM_ALL;

    auto numa_nodes = tp.get_memory_only_numa_nodes();
    ssize_t capacity = tp.get_capacity(numa_nodes);
    ASSERT_GT(capacity, 0);

    ASSERT_EQ(capacity, memkind_get_capacity(kind));
}

TEST_F(MemkindGetCapacityTestsDaxKmem,
       test_tc_memkind_verify_capacity_dax_kmem_interleave)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    memkind_t kind = MEMKIND_DAX_KMEM_INTERLEAVE;

    auto numa_nodes = tp.get_memory_only_numa_nodes();
    ssize_t capacity = tp.get_capacity(numa_nodes);
    ASSERT_GT(capacity, 0);

    ASSERT_EQ(capacity, memkind_get_capacity(kind));
}

TEST_P(MemkindGetCapacityTestsPreferredParam,
       test_tc_memkind_verify_capacity_preferred_kinds)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);

    // Expect the function to fail (unsupported kinds)
    int expected_ret_code = -1;
    ASSERT_EQ(expected_ret_code, memkind_get_capacity(memory_kind));
}
