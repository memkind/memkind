/*
 * Copyright (C) 2019 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "memkind.h"
#include "common.h"
#include "vector"
#include <stdio.h>

class MemkindTransferAllocationTests: public ::testing::Test
{
protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindTransferAllocationTests, test_TC_MEMKIND_NULLTransfer)
{
    void *ptr = memkind_transfer_allocation(nullptr, nullptr);
    ASSERT_EQ(ptr, nullptr);
    ptr = memkind_transfer_allocation(MEMKIND_REGULAR, nullptr);
    ASSERT_EQ(ptr, nullptr);
}

TEST_F(MemkindTransferAllocationTests, test_TC_MEMKIND_TCACHE_test)
{
    void *ptr = memkind_malloc(MEMKIND_DEFAULT, 512);
    ASSERT_NE(ptr, nullptr);
    void *new_ptr = memkind_transfer_allocation(MEMKIND_DEFAULT, ptr);
    ASSERT_EQ(new_ptr, nullptr);
}

TEST_F(MemkindTransferAllocationTests,
       test_TC_MEMKIND_TCACHE_Default_kind_success)
{
    std::vector<void *> alloc_vec;
    const size_t number_of_malloc = 10000000;
    alloc_vec.reserve(2*number_of_malloc);
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
            memkind_free(MEMKIND_DEFAULT,alloc_vec.at(i));
            alloc_vec.at(i) = nullptr;
        }
        ptr = memkind_malloc(MEMKIND_DEFAULT, 300);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, 'a', 300);
        alloc_vec.push_back(ptr);
    }

    for (i = 0; i < alloc_vec.size(); ++i) {
        void *new_ptr = memkind_transfer_allocation(MEMKIND_DEFAULT, alloc_vec.at(i));
        if (new_ptr) {
            alloc_vec.at(i) = new_ptr;
            count_mem_transfer++;
        }
    }

    ASSERT_NE(count_mem_transfer, 0U);

    for(auto const &val: alloc_vec) {
        memkind_free(MEMKIND_DEFAULT, val);
    }
}

TEST_F(MemkindTransferAllocationTests,
       test_TC_MEMKIND_TCACHE_REGULAR_kind_success)
{
    std::vector<void *> alloc_vec;
    const size_t number_of_malloc = 10000000;
    alloc_vec.reserve(2*number_of_malloc);
    size_t count_mem_transfer = 0;
    void *ptr;
    size_t i;

    for (i = 0; i < number_of_malloc ; ++i) {
        ptr = memkind_malloc(MEMKIND_REGULAR, 150);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, 'a', 150);
        alloc_vec.push_back(ptr);
    }

    for (i = 0; i < number_of_malloc; ++i) {
        if (i % 2 == 0) {
            memkind_free(MEMKIND_REGULAR,alloc_vec.at(i));
            alloc_vec.at(i) = nullptr;
        }
        ptr = memkind_malloc(MEMKIND_REGULAR, 300);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, 'a', 300);
        alloc_vec.push_back(ptr);
    }

    for (i = 0; i < alloc_vec.size(); ++i) {
        void *new_ptr = memkind_transfer_allocation(nullptr, alloc_vec.at(i));
        if (new_ptr) {
            alloc_vec.at(i) = new_ptr;
            count_mem_transfer++;
        }
    }

    ASSERT_NE(count_mem_transfer, 0U);

    for(auto const &val: alloc_vec) {
        memkind_free(MEMKIND_REGULAR, val);
    }
}
