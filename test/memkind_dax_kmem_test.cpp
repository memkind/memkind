// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2021 Intel Corporation. */

#include <memkind.h>

#include <algorithm>
#include <numaif.h>
#include <string>
#include <unistd.h>

#include "TestPrereq.hpp"
#include "common.h"
#include "proc_stat.h"
#include "sys/sysinfo.h"
#include "sys/types.h"

class MemkindDaxKmemFunctionalTests: public ::testing::Test
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

class MemkindDaxKmemFunctionalTestsAll: public ::testing::Test
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

class MemkindDaxKmemFunctionalTestsPreferred: public ::testing::Test
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

class MemkindDaxKmemTestsParam: public ::Memkind_Param_Test
{
protected:
    TestPrereq tp;

    void SetUp()
    {
        Memkind_Param_Test::SetUp();
        if (!tp.is_DAX_KMEM_supported()) {
            GTEST_SKIP() << "DAX KMEM is required." << std::endl;
        }
        if (tp.is_kind_preferred(memory_kind) && !tp.is_preferred_supported()) {
            GTEST_SKIP() << "Exactly one closest DAX KMEM node is required."
                         << std::endl;
        }
    }
};

INSTANTIATE_TEST_CASE_P(KindParam, MemkindDaxKmemTestsParam,
                        ::testing::Values(MEMKIND_DAX_KMEM,
                                          MEMKIND_DAX_KMEM_ALL,
                                          MEMKIND_DAX_KMEM_PREFERRED,
                                          MEMKIND_DAX_KMEM_INTERLEAVE));

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_free_with_NULL_kind_4096_bytes)
{
    void *ptr = memkind_malloc(memory_kind, 4 * KB);
    ASSERT_NE(nullptr, ptr);
    memkind_free(nullptr, ptr);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_zero)
{
    void *test1 = memkind_malloc(memory_kind, 0);
    ASSERT_EQ(test1, nullptr);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_size_max)
{
    errno = 0;
    void *test1 = memkind_malloc(memory_kind, SIZE_MAX);
    ASSERT_EQ(test1, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_calloc_zero)
{
    void *test = memkind_calloc(memory_kind, 0, 100);
    ASSERT_EQ(test, nullptr);

    test = memkind_calloc(memory_kind, 100, 0);
    ASSERT_EQ(test, nullptr);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_calloc_size_max)
{
    const size_t size = SIZE_MAX;
    const size_t num = 1;
    errno = 0;

    void *test = memkind_calloc(memory_kind, size, num);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_calloc_num_max)
{
    const size_t size = 10;
    const size_t num = SIZE_MAX;
    errno = 0;

    void *test = memkind_calloc(memory_kind, size, num);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc)
{
    const size_t size1 = 512;
    const size_t size2 = 1 * KB;
    char *default_str = nullptr;

    default_str = (char *)memkind_realloc(memory_kind, default_str, size1);
    ASSERT_NE(nullptr, default_str);

    sprintf(default_str, "memkind_realloc with size %zu\n", size1);
    printf("%s", default_str);

    default_str = (char *)memkind_realloc(memory_kind, default_str, size2);
    ASSERT_NE(nullptr, default_str);

    sprintf(default_str, "memkind_realloc with size %zu\n", size2);
    printf("%s", default_str);

    memkind_free(memory_kind, default_str);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_zero)
{
    const size_t size = 1 * KB;

    void *test = memkind_malloc(memory_kind, size);
    ASSERT_NE(test, nullptr);

    void *new_test = memkind_realloc(memory_kind, test, 0);
    ASSERT_EQ(new_test, nullptr);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_size_max)
{
    const size_t size = 1 * KB;

    void *test = memkind_malloc(memory_kind, size);
    ASSERT_NE(test, nullptr);

    errno = 0;
    void *new_test = memkind_realloc(memory_kind, test, SIZE_MAX);
    ASSERT_EQ(new_test, nullptr);
    ASSERT_EQ(errno, ENOMEM);

    memkind_free(memory_kind, test);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_size_zero)
{
    const size_t size = 1 * KB;
    void *test = nullptr;
    void *new_test = nullptr;
    const size_t iteration = 100;

    for (unsigned i = 0; i < iteration; ++i) {
        test = memkind_malloc(memory_kind, size);
        ASSERT_NE(test, nullptr);
        errno = 0;
        new_test = memkind_realloc(memory_kind, test, 0);
        ASSERT_EQ(new_test, nullptr);
        ASSERT_EQ(errno, 0);
    }
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_nullptr)
{
    const size_t size = 1 * KB;
    void *test = nullptr;

    test = memkind_realloc(memory_kind, test, size);
    ASSERT_NE(test, nullptr);

    memkind_free(memory_kind, test);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_increase_decrease_size)
{
    size_t size = 1 * KB;
    const char val[] = "test_TC_MEMKIND_PmemReallocIncreaseSizeNullKindVariant";
    int status;

    char *test1 = (char *)memkind_malloc(memory_kind, size);
    ASSERT_NE(test1, nullptr);

    sprintf(test1, "%s", val);

    size = 2 * KB;
    char *test2 = (char *)memkind_realloc(nullptr, test1, size);
    ASSERT_NE(test2, nullptr);
    status = memcmp(val, test2, sizeof(val));
    ASSERT_EQ(status, 0);

    size = 512;
    char *test3 = (char *)memkind_realloc(nullptr, test2, size);
    ASSERT_NE(test3, nullptr);
    status = memcmp(val, test3, sizeof(val));
    ASSERT_EQ(status, 0);

    memkind_free(memory_kind, test3);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_free_nullptr)
{
    const int n_iter = 5000;

    for (int i = 0; i < n_iter; ++i) {
        memkind_free(memory_kind, nullptr);
    }
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_posix_memalign_alignment_less_than_void)
{
    void *test = nullptr;
    const size_t size = 32;
    const size_t wrong_alignment = 4;

    errno = 0;
    int ret = memkind_posix_memalign(memory_kind, &test, wrong_alignment, size);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(ret, EINVAL);
    ASSERT_EQ(test, nullptr);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_posix_memalign_alignment_not_pow2)
{
    void *test = nullptr;
    const size_t size = 32;
    const size_t wrong_alignment = sizeof(void *) + 1;

    errno = 0;
    int ret = memkind_posix_memalign(memory_kind, &test, wrong_alignment, size);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(ret, EINVAL);
    ASSERT_EQ(test, nullptr);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_posix_memalign_correct_alignment)
{
    void *test = nullptr;
    const size_t size = 32;
    const size_t alignment = sizeof(void *);

    errno = 0;
    int ret = memkind_posix_memalign(memory_kind, &test, alignment, size);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(test, nullptr);

    memkind_free(nullptr, test);
}

TEST_F(MemkindDaxKmemFunctionalTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_until_full_numa)
{
    ProcStat stat;
    const size_t alloc_size = 100 * MB;
    std::set<void *> allocations;
    size_t numa_size;
    int numa_id = -1;
    const int n_swap_alloc = 20;

    void *ptr = memkind_malloc(MEMKIND_DAX_KMEM, alloc_size);
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', alloc_size);
    allocations.insert(ptr);

    get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    numa_size = numa_node_size64(numa_id, nullptr);

    while (numa_size > alloc_size * allocations.size()) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);
    }

    size_t init_swap = stat.get_used_swap_space_size_bytes();
    for (int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);
    }

    ASSERT_GE(stat.get_used_swap_space_size_bytes(), init_swap);

    for (auto const &ptr : allocations) {
        memkind_free(MEMKIND_DAX_KMEM, ptr);
    }
}

TEST_F(MemkindDaxKmemFunctionalTestsAll,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_ALL_alloc_until_full_numa)
{
    auto dax_kmem_nodes = tp.get_memory_only_numa_nodes();
    if (dax_kmem_nodes.size() < 2)
        GTEST_SKIP() << "This test requires minimum 2 DAX KMEM nodes";

    ProcStat stat;
    void *ptr;
    const size_t alloc_size = 100 * MB;
    std::set<void *> allocations;
    size_t sum_of_dax_kmem_free_space = tp.get_free_space(dax_kmem_nodes);
    int numa_id = -1;
    const int n_swap_alloc = 20;

    while (sum_of_dax_kmem_free_space > alloc_size * allocations.size()) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM_ALL, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);

        get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
        ASSERT_TRUE(dax_kmem_nodes.find(numa_id) != dax_kmem_nodes.end());
    }

    size_t init_swap = stat.get_used_swap_space_size_bytes();
    for (int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM_ALL, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);
    }

    ASSERT_GE(stat.get_used_swap_space_size_bytes(), init_swap);

    for (auto const &ptr : allocations) {
        memkind_free(MEMKIND_DAX_KMEM_ALL, ptr);
    }
}

TEST_F(MemkindDaxKmemFunctionalTestsPreferred,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_PREFFERED_alloc_until_full_numa)
{
    if (!tp.is_preferred_supported()) {
        GTEST_SKIP() << "Exactly one closest DAX KMEM node is required."
                     << std::endl;
    }

    ProcStat stat;
    const size_t alloc_size = 100 * MB;
    std::set<void *> allocations;
    size_t numa_size;
    int numa_id = -1;
    const int n_swap_alloc = 20;

    void *ptr = memkind_malloc(MEMKIND_DAX_KMEM_PREFERRED, alloc_size);
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', alloc_size);
    allocations.insert(ptr);

    get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    int process_cpu = sched_getcpu();
    int process_node = numa_node_of_cpu(process_cpu);

    auto dax_kmem_nodes = tp.get_memory_only_numa_nodes();
    auto closest_numa_ids =
        tp.get_closest_numa_nodes(process_node, dax_kmem_nodes);

    numa_size = numa_node_size64(numa_id, nullptr);

    while (0.99 * numa_size > alloc_size * allocations.size()) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM_PREFERRED, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);

        get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
        ASSERT_TRUE(closest_numa_ids.find(numa_id) != closest_numa_ids.end());
    }

    for (int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(MEMKIND_DAX_KMEM_PREFERRED, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);
    }

    ASSERT_EQ(stat.get_used_swap_space_size_bytes(), 0U);

    for (auto const &ptr : allocations) {
        memkind_free(MEMKIND_DAX_KMEM_PREFERRED, ptr);
    }
}

TEST_F(MemkindDaxKmemFunctionalTestsPreferred,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_PREFFERED_check_prerequisities)
{
    if (tp.is_preferred_supported()) {
        GTEST_SKIP() << "More than one closest DAX KMEM nodes are required."
                     << std::endl;
    }
    const size_t alloc_size = 100 * MB;
    void *ptr = memkind_malloc(MEMKIND_DAX_KMEM_PREFERRED, alloc_size);
    ASSERT_EQ(nullptr, ptr);
}
