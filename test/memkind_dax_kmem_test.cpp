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

#include <memkind.h>
#include "common.h"

class MemkindDaxKmemTests: public ::testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindDaxKmemTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_free_with_NULL_kind_4096_bytes)
{
    void *ptr = memkind_malloc(MEMKIND_DAX_KMEM, 4096);
    ASSERT_NE(nullptr, ptr);
    memkind_free(nullptr, ptr);
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_zero)
{
    void *test1 = nullptr;

    test1 = memkind_malloc(MEMKIND_DAX_KMEM, 0);
    ASSERT_EQ(test1, nullptr);
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_alloc_size_max)
{
    void *test1 = nullptr;

    errno = 0;
    test1 = memkind_malloc(MEMKIND_DAX_KMEM, SIZE_MAX);
    ASSERT_EQ(test1, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_calloc_zero)
{
    void *test = nullptr;

    test = memkind_calloc(MEMKIND_DAX_KMEM, 0, 100);
    ASSERT_EQ(test, nullptr);

    test = memkind_calloc(MEMKIND_DAX_KMEM, 100, 0);
    ASSERT_EQ(test, nullptr);
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_calloc_size_max)
{
    void *test = nullptr;
    size_t size = SIZE_MAX;
    size_t num = 1;
    errno = 0;

    test = memkind_calloc(MEMKIND_DAX_KMEM, size, num);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_calloc_num_max)
{
    void *test = nullptr;
    size_t size = 10;
    size_t num = SIZE_MAX;
    errno = 0;

    test = memkind_calloc(MEMKIND_DAX_KMEM, size, num);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc)
{
    const size_t size1 = 512;
    const size_t size2 = 1 * KB;
    char *default_str = nullptr;

    default_str = (char *)memkind_realloc(MEMKIND_DAX_KMEM, default_str, size1);
    ASSERT_NE(nullptr, default_str);

    sprintf(default_str, "memkind_realloc MEMKIND_DAX_KMEM with size %zu\n", size1);
    printf("%s", default_str);

    default_str = (char *)memkind_realloc(MEMKIND_DAX_KMEM, default_str, size2);
    ASSERT_NE(nullptr, default_str);

    sprintf(default_str, "memkind_realloc MEMKIND_DAX_KMEM with size %zu\n", size2);
    printf("%s", default_str);

    memkind_free(MEMKIND_DAX_KMEM, default_str);
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_zero)
{
    size_t size = 1 * KB;
    void *test = nullptr;
    void *new_test = nullptr;

    test = memkind_malloc(MEMKIND_DAX_KMEM, size);
    ASSERT_NE(test, nullptr);

    new_test = memkind_realloc(MEMKIND_DAX_KMEM, test, 0);
    ASSERT_EQ(new_test, nullptr);
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_size_max)
{
    size_t size = 1 * KB;
    void *test = nullptr;
    void *new_test = nullptr;

    test = memkind_malloc(MEMKIND_DAX_KMEM, size);
    ASSERT_NE(test, nullptr);
    errno = 0;
    new_test = memkind_realloc(MEMKIND_DAX_KMEM, test, SIZE_MAX);
    ASSERT_EQ(new_test, nullptr);
    ASSERT_EQ(errno, ENOMEM);

    memkind_free(MEMKIND_DAX_KMEM, test);
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_size_zero)
{
    size_t size = 1 * KB;
    void *test = nullptr;
    void *new_test = nullptr;
    const size_t iteration = 100;

    for (unsigned i = 0; i < iteration; ++i) {
        test = memkind_malloc(MEMKIND_DAX_KMEM, size);
        ASSERT_NE(test, nullptr);
        errno = 0;
        new_test = memkind_realloc(MEMKIND_DAX_KMEM, test, 0);
        ASSERT_EQ(new_test, nullptr);
        ASSERT_EQ(errno, 0);
    }
}

TEST_F(MemkindDaxKmemTests, test_TC_MEMKIND_MEMKIND_DAX_KMEM_realloc_nullptr)
{
    size_t size = 1 * KB;
    void *test = nullptr;

    test = memkind_realloc(MEMKIND_DAX_KMEM, test, size);
    ASSERT_NE(test, nullptr);

    memkind_free(MEMKIND_DAX_KMEM, test);
}
