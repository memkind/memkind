// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include "common.h"
#include "memkind.h"
#include "vector"
#include <stdio.h>

class MemkindDefragReallocateTests: public ::testing::Test
{
protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindDefragReallocateTests, test_TC_MEMKIND_NULLTransfer)
{
    void *ptr = memkind_defrag_reallocate(nullptr, nullptr);
    ASSERT_EQ(ptr, nullptr);
    ptr = memkind_defrag_reallocate(MEMKIND_REGULAR, nullptr);
    ASSERT_EQ(ptr, nullptr);
}

TEST_F(MemkindDefragReallocateTests, test_TC_MEMKIND_Negative_test)
{
    void *ptr = memkind_malloc(MEMKIND_DEFAULT, 512);
    ASSERT_NE(ptr, nullptr);
    void *new_ptr = memkind_defrag_reallocate(MEMKIND_DEFAULT, ptr);
    ASSERT_EQ(new_ptr, nullptr);
}

TEST_F(MemkindDefragReallocateTests,
       test_TC_MEMKIND_TCACHE_Default_kind_success)
{
    std::vector<void *> alloc_vec;
    const size_t number_of_malloc = 10000000;
    alloc_vec.reserve(2 * number_of_malloc);
    size_t count_mem_transfer = 0;
    void *ptr;
    size_t i;

    for (i = 0; i < number_of_malloc; ++i) {
        ptr = memkind_malloc(MEMKIND_DEFAULT, 150);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, 'a', 150);
        alloc_vec.push_back(ptr);
    }

    for (i = 0; i < number_of_malloc; ++i) {
        if (i % 2 == 0) {
            memkind_free(MEMKIND_DEFAULT, alloc_vec.at(i));
            alloc_vec.at(i) = nullptr;
        }
        ptr = memkind_malloc(MEMKIND_DEFAULT, 300);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, 'a', 300);
        alloc_vec.push_back(ptr);
    }

    for (i = 0; i < alloc_vec.size(); ++i) {
        void *new_ptr =
            memkind_defrag_reallocate(MEMKIND_DEFAULT, alloc_vec.at(i));
        if (new_ptr) {
            alloc_vec.at(i) = new_ptr;
            count_mem_transfer++;
        }
    }

    ASSERT_NE(count_mem_transfer, 0U);

    for (auto const &val : alloc_vec) {
        memkind_free(MEMKIND_DEFAULT, val);
    }
}

TEST_F(MemkindDefragReallocateTests,
       test_TC_MEMKIND_TCACHE_REGULAR_kind_success)
{
    std::vector<void *> alloc_vec;
    const size_t number_of_malloc = 10000000;
    alloc_vec.reserve(2 * number_of_malloc);
    size_t count_mem_transfer = 0;
    void *ptr;
    size_t i;

    for (i = 0; i < number_of_malloc; ++i) {
        ptr = memkind_malloc(MEMKIND_REGULAR, 150);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, 'a', 150);
        alloc_vec.push_back(ptr);
    }

    for (i = 0; i < number_of_malloc; ++i) {
        if (i % 2 == 0) {
            memkind_free(MEMKIND_REGULAR, alloc_vec.at(i));
            alloc_vec.at(i) = nullptr;
        }
        ptr = memkind_malloc(MEMKIND_REGULAR, 300);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, 'a', 300);
        alloc_vec.push_back(ptr);
    }

    for (i = 0; i < alloc_vec.size(); ++i) {
        void *new_ptr = memkind_defrag_reallocate(nullptr, alloc_vec.at(i));
        if (new_ptr) {
            alloc_vec.at(i) = new_ptr;
            count_mem_transfer++;
        }
    }

    ASSERT_NE(count_mem_transfer, 0U);

    for (auto const &val : alloc_vec) {
        memkind_free(MEMKIND_REGULAR, val);
    }
}
