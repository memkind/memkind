// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind.h>
#include <memkind/internal/memkind_private.h>

#include "TestPrereq.hpp"
#include "common.h"

class MemkindGetTotalMemoryTests: public ::testing::Test
{
protected:
    TestPrereq tp;
};

class MemkindGetTotalMemoryTestsDaxKmem: public ::testing::Test
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

TEST_F(MemkindGetTotalMemoryTests, test_tc_memkind_verify_total_memory_fsdax)
{
    const char *dir = "/tmp";
    long long max_size = 2 * MEMKIND_PMEM_MIN_SIZE;
    struct memkind *kind = NULL;
    memkind_create_pmem(dir, max_size, &kind);

    ASSERT_EQ(max_size, memkind_get_total_memory(kind));
    memkind_destroy_kind(kind);
}

TEST_F(MemkindGetTotalMemoryTests, test_tc_memkind_verify_total_memory_default)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    auto numa_nodes = tp.get_all_numa_nodes();
    long long total_memory = 0;

    for (auto node : numa_nodes) {
        long long curr_node_size = numa_node_size64(node, NULL);
        ASSERT_GT(curr_node_size, 0);
        total_memory += curr_node_size;
    }
    ASSERT_EQ(total_memory, memkind_get_total_memory(MEMKIND_DEFAULT));
}

TEST_F(MemkindGetTotalMemoryTests, test_tc_memkind_verify_total_memory_hugetlb)
{
    memkind_t kind = MEMKIND_HUGETLB;
    size_t size = 1024 * 1024;
    void *ptr = memkind_malloc(kind, size);
    if (ptr == NULL) {
        GTEST_SKIP()
            << "Cannot allocate MEMKIND_HUGETLB. Check if hugepages are available."
            << std::endl;
    }
    memkind_free(kind, ptr);

    int status = numa_available();
    ASSERT_EQ(status, 0);
    auto numa_nodes = tp.get_all_numa_nodes();
    long long total_memory = 0;

    for (auto node : numa_nodes) {
        long long curr_node_size = numa_node_size64(node, NULL);
        ASSERT_GT(curr_node_size, 0);
        total_memory += curr_node_size;
    }
    ASSERT_EQ(total_memory, memkind_get_total_memory(kind));
}

TEST_F(MemkindGetTotalMemoryTests,
       test_tc_memkind_verify_total_memory_interleave)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    auto numa_nodes = tp.get_all_numa_nodes();
    long long total_memory = 0;

    for (auto node : numa_nodes) {
        long long curr_node_size = numa_node_size64(node, NULL);
        ASSERT_GT(curr_node_size, 0);
        total_memory += curr_node_size;
    }
    ASSERT_EQ(total_memory, memkind_get_total_memory(MEMKIND_INTERLEAVE));
}

TEST_F(MemkindGetTotalMemoryTests, test_tc_memkind_verify_total_memory_regular)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    auto numa_nodes = tp.get_regular_numa_nodes();
    long long total_memory = 0;

    for (auto node : numa_nodes) {
        long long curr_node_size = numa_node_size64(node, NULL);
        ASSERT_GT(curr_node_size, 0);
        total_memory += curr_node_size;
    }

    memkind_t kind = MEMKIND_REGULAR;
    void *ptr = memkind_malloc(kind, KB);
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', KB);
    ASSERT_EQ(total_memory, memkind_get_total_memory(kind));
    memkind_free(kind, ptr);
}

TEST_F(MemkindGetTotalMemoryTestsDaxKmem,
       test_tc_memkind_verify_total_memory_dax_kmem)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    long long total_memory = 0;
    ASSERT_EQ(total_memory, 0);
    memkind_t kind = MEMKIND_DAX_KMEM;

    int process_cpu = sched_getcpu();
    int process_node = numa_node_of_cpu(process_cpu);
    ASSERT_EQ(status, 0);

    auto dax_kmem_nodes = tp.get_memory_only_numa_nodes();
    auto numa_nodes = tp.get_closest_numa_nodes(process_node, dax_kmem_nodes);

    for (auto node : numa_nodes) {
        long long curr_node_size = numa_node_size64(node, NULL);
        ASSERT_GT(curr_node_size, 0);
        total_memory += curr_node_size;
    }

    ASSERT_EQ(total_memory, memkind_get_total_memory(kind));
}

TEST_F(MemkindGetTotalMemoryTestsDaxKmem,
       test_tc_memkind_verify_total_memory_dax_kmem_all)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    long long total_memory = 0;
    ASSERT_EQ(total_memory, 0);
    memkind_t kind = MEMKIND_DAX_KMEM_ALL;

    auto numa_nodes = tp.get_memory_only_numa_nodes();

    for (auto node : numa_nodes) {
        long long curr_node_size = numa_node_size64(node, NULL);
        ASSERT_GT(curr_node_size, 0);
        total_memory += curr_node_size;
    }

    ASSERT_EQ(total_memory, memkind_get_total_memory(kind));
}

TEST_F(MemkindGetTotalMemoryTestsDaxKmem,
       test_tc_memkind_verify_total_memory_dax_kmem_interleave)
{
    int status = numa_available();
    ASSERT_EQ(status, 0);
    long long total_memory = 0;
    ASSERT_EQ(total_memory, 0);
    memkind_t kind = MEMKIND_DAX_KMEM_INTERLEAVE;

    auto numa_nodes = tp.get_memory_only_numa_nodes();

    for (auto node : numa_nodes) {
        long long curr_node_size = numa_node_size64(node, NULL);
        ASSERT_GT(curr_node_size, 0);
        total_memory += curr_node_size;
    }

    ASSERT_EQ(total_memory, memkind_get_total_memory(kind));
}
