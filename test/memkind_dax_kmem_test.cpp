// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2021 Intel Corporation. */

#include <algorithm>
#include <numaif.h>
#include <string>
#include <unistd.h>

#include "common.h"
#include "proc_stat.h"
#include "sys/types.h"
#include "sys/sysinfo.h"

// TODO unify this
class MemkindDaxKmemFunctionalTestsAll: public ::Memkind_Test
{
protected:
    void SetUp()
    {
        kind = MEMKIND_DAX_KMEM_ALL;
        if (!prereq.is_DAX_KMEM_supported()) {
            GTEST_SKIP() << "DAX KMEM is required." << std::endl;
        }
        Memkind_Test::SetUp();
    }
};
// TODO unify this
class MemkindDaxKmemFunctionalTestsSingle: public ::Memkind_Test
{
protected:
    void SetUp()
    {
        kind = MEMKIND_DAX_KMEM;
        if (!prereq.is_DAX_KMEM_supported()) {
            GTEST_SKIP() << "DAX KMEM is required." << std::endl;
        }
        Memkind_Test::SetUp();
    }
};

class MemkindDaxKmemTestsParam: public ::Memkind_Param_Test
{
protected:
    void SetUp()
    {
        if (!prereq.is_DAX_KMEM_supported()) {
            GTEST_SKIP() << "DAX KMEM is required." << std::endl;
        }
        Memkind_Param_Test::SetUp();
    }
};

INSTANTIATE_TEST_CASE_P(
    KindParam, MemkindDaxKmemTestsParam,
    ::testing::Values(MEMKIND_DAX_KMEM, MEMKIND_DAX_KMEM_ALL,
                      MEMKIND_DAX_KMEM_PREFERRED, MEMKIND_DAX_KMEM_INTERLEAVE));

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_free_with_NULL_kind_4096_bytes)
{
    void *ptr = memkind_malloc(kind, 4 * KB);
    ASSERT_NE(nullptr, ptr);
    memkind_free(nullptr, ptr);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_zero)
{
    void *test1 = memkind_malloc(kind, 0);
    ASSERT_EQ(test1, nullptr);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_size_max)
{
    errno = 0;
    void *test1 = memkind_malloc(kind, SIZE_MAX);
    ASSERT_EQ(test1, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_calloc_zero)
{
    void *test = memkind_calloc(kind, 0, 100);
    ASSERT_EQ(test, nullptr);

    test = memkind_calloc(kind, 100, 0);
    ASSERT_EQ(test, nullptr);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_calloc_size_max)
{
    const size_t size = SIZE_MAX;
    const size_t num = 1;
    errno = 0;

    void *test = memkind_calloc(kind, size, num);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_calloc_num_max)
{
    const size_t size = 10;
    const size_t num = SIZE_MAX;
    errno = 0;

    void *test = memkind_calloc(kind, size, num);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc)
{
    const size_t size1 = 512;
    const size_t size2 = 1 * KB;
    char *default_str = nullptr;

    default_str = (char *)memkind_realloc(kind, default_str, size1);
    ASSERT_NE(nullptr, default_str);

    sprintf(default_str, "memkind_realloc with size %zu\n", size1);
    printf("%s", default_str);

    default_str = (char *)memkind_realloc(kind, default_str, size2);
    ASSERT_NE(nullptr, default_str);

    sprintf(default_str, "memkind_realloc with size %zu\n", size2);
    printf("%s", default_str);

    memkind_free(kind, default_str);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_zero)
{
    const size_t size = 1 * KB;

    void *test = memkind_malloc(kind, size);
    ASSERT_NE(test, nullptr);

    void *new_test = memkind_realloc(kind, test, 0);
    ASSERT_EQ(new_test, nullptr);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_size_max)
{
    const size_t size = 1 * KB;

    void *test = memkind_malloc(kind, size);
    ASSERT_NE(test, nullptr);

    errno = 0;
    void *new_test = memkind_realloc(kind, test, SIZE_MAX);
    ASSERT_EQ(new_test, nullptr);
    ASSERT_EQ(errno, ENOMEM);

    memkind_free(kind, test);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_size_zero)
{
    const size_t size = 1 * KB;
    void *test = nullptr;
    void *new_test = nullptr;
    const size_t iteration = 100;

    for (unsigned i = 0; i < iteration; ++i) {
        test = memkind_malloc(kind, size);
        ASSERT_NE(test, nullptr);
        errno = 0;
        new_test = memkind_realloc(kind, test, 0);
        ASSERT_EQ(new_test, nullptr);
        ASSERT_EQ(errno, 0);
    }
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_nullptr)
{
    const size_t size = 1 * KB;
    void *test = nullptr;

    test = memkind_realloc(kind, test, size);
    ASSERT_NE(test, nullptr);

    memkind_free(kind, test);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_increase_decrease_size)
{
    size_t size = 1 * KB;
    const char val[] = "test_TC_MEMKIND_PmemReallocIncreaseSizeNullKindVariant";
    int status;

    char *test1 = (char *)memkind_malloc(kind, size);
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

    memkind_free(kind, test3);
}

TEST_P(MemkindDaxKmemTestsParam, test_TC_MEMKIND_MEMKIND_DAX_KMEM_free_nullptr)
{
    const int n_iter = 5000;

    for (int i = 0; i < n_iter; ++i) {
        memkind_free(kind, nullptr);
    }
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_posix_memalign_alignment_less_than_void)
{
    void *test = nullptr;
    const size_t size = 32;
    const size_t wrong_alignment = 4;

    errno = 0;
    int ret = memkind_posix_memalign(kind, &test, wrong_alignment, size);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(ret, EINVAL);
    ASSERT_EQ(test, nullptr);
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_posix_memalign_alignment_not_pow2)
{
    void *test = nullptr;
    const size_t size = 32;
    const size_t wrong_alignment = sizeof(void *)+1;

    errno = 0;
    int ret = memkind_posix_memalign(kind, &test, wrong_alignment, size);
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
    int ret = memkind_posix_memalign(kind, &test, alignment, size);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(test, nullptr);

    memkind_free(nullptr, test);
}

TEST_F(MemkindDaxKmemFunctionalTestsSingle,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_until_full_numa)
{
    ProcStat stat;
    const size_t alloc_size = 100 * MB;
    const size_t alloc_size_swap = 1 * MB;
    std::set<void *> allocations;
    size_t numa_size;
    int numa_id = -1;
    const int n_swap_alloc = 20;

    void *ptr = memkind_malloc(kind, alloc_size);
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', alloc_size);
    allocations.insert(ptr);

    get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    numa_size = numa_node_size64(numa_id, nullptr);

    while (numa_size > alloc_size * allocations.size()) {
        ptr = memkind_malloc(kind, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);
    }

    size_t init_swap = stat.get_used_swap_space_size_bytes();
    for(int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(kind, alloc_size_swap);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size_swap);
        allocations.insert(ptr);
    }

    ASSERT_GE(stat.get_used_swap_space_size_bytes(), init_swap);

    for (auto const &ptr: allocations) {
        memkind_free(kind, ptr);
    }
}

TEST_F(MemkindDaxKmemFunctionalTestsAll,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_ALL_alloc_until_full_numa)
{
    auto dax_kmem_nodes = prereq.get_memory_only_numa_nodes();
    if (dax_kmem_nodes.size() < 2)
        GTEST_SKIP() << "This test requires minimum 2 DAX KMEM nodes";

    ProcStat stat;
    void *ptr;
    const size_t alloc_size = 100 * MB;
    const size_t alloc_size_swap = 1 * MB;
    std::set<void *> allocations;
    size_t sum_of_free_space = prereq.get_free_space(dax_kmem_nodes);
    int numa_id = -1;
    const int n_swap_alloc = 20;
    size_t sum_of_alloc = 0;

    while (sum_of_free_space > sum_of_alloc) {
        ptr = memkind_malloc(kind, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);
        sum_of_alloc += alloc_size;

        int ret = get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
        ASSERT_EQ(ret, 0);

        // signal the moment when we actually reach free space of DAX KMEM Nodes
        if (dax_kmem_nodes.find(numa_id) == dax_kmem_nodes.end()) {
            break;
        }
    }
    ASSERT_GE(sum_of_alloc, 0.99*sum_of_free_space);

    size_t init_swap = stat.get_used_swap_space_size_bytes();
    for(int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(kind, alloc_size_swap);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size_swap);
        allocations.insert(ptr);
    }

    ASSERT_GE(stat.get_used_swap_space_size_bytes(), init_swap);

    for (auto const &ptr: allocations) {
        memkind_free(kind, ptr);
    }
}

TEST_F(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_PREFERRED_alloc_until_full_numa)
{
    if (!prereq.is_kind_preferred(kind))
        GTEST_SKIP() << "Preferred memory kind is required."<< std::endl;
    else if (!prereq.is_preferred_supported()) {
        GTEST_SKIP() << "Exactly one closest DAX KMEM node is required."
                     << std::endl;
    }

    ProcStat stat;
    const size_t alloc_size = 100 * MB;
    const size_t alloc_size_swap = 1 * MB;
    std::set<void *> allocations;
    size_t numa_size;
    int numa_id = -1;
    const int n_swap_alloc = 20;

    void *ptr = memkind_malloc(kind, alloc_size);
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', alloc_size);
    allocations.insert(ptr);

    get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    int process_cpu = sched_getcpu();
    int process_node = numa_node_of_cpu(process_cpu);

    auto dax_kmem_nodes = prereq.get_memory_only_numa_nodes();
    auto closest_numa_ids = prereq.get_closest_numa_nodes(process_node,
                                                          dax_kmem_nodes);

    numa_size = numa_node_size64(numa_id, nullptr);

    while (0.99 * numa_size > alloc_size * allocations.size()) {
        ptr = memkind_malloc(kind, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);

        get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
        ASSERT_TRUE(closest_numa_ids.find(numa_id) != closest_numa_ids.end());
    }

    for (int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(kind, alloc_size_swap);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size_swap);
        allocations.insert(ptr);
    }

    ASSERT_EQ(stat.get_used_swap_space_size_bytes(), 0U);

    for (auto const &ptr: allocations) {
        memkind_free(kind, ptr);
    }
}

TEST_P(MemkindDaxKmemTestsParam,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_PREFERRED_check_prerequisities)
{
    if (!prereq.is_kind_preferred(kind))
        GTEST_SKIP() << "Preferred memory kind is required."<< std::endl;
    else if (prereq.is_preferred_supported()) {
        GTEST_SKIP() << "More than one closest DAX KMEM nodes are required." <<
                     std::endl;
    }

    const size_t alloc_size = 100 * MB;
    void *ptr = memkind_malloc(kind, alloc_size);
    ASSERT_EQ(nullptr, ptr);
}
