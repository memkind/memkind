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

extern const char  *PMEM_DIR;

class MemkindDetectKindTests: public ::testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindDetectKindTests, test_TC_MEMKIND_DetectKindNullPtr)
{
    struct memkind *temp_kind = memkind_detect_kind(nullptr);
    ASSERT_EQ(nullptr, temp_kind);
}

TEST_F(MemkindDetectKindTests, test_TC_MEMKIND_DetectKindPmemKind)
{
    memkind_t pmem_kind_temp = nullptr;
    int err = memkind_create_pmem(PMEM_DIR, 0, &pmem_kind_temp);
    ASSERT_EQ(0, err);
    ASSERT_NE(nullptr, pmem_kind_temp);

    void *ptr = memkind_malloc(pmem_kind_temp, 512);
    ASSERT_NE(nullptr, ptr);

    struct memkind *temp_kind = memkind_detect_kind(ptr);
    ASSERT_EQ(pmem_kind_temp, temp_kind);

    memkind_free(pmem_kind_temp, ptr);

    err = memkind_destroy_kind(pmem_kind_temp);
    ASSERT_EQ(0, err);
}

TEST_F(MemkindDetectKindTests, test_TC_MEMKIND_DetectKindRegularKind)
{
    void *ptr = memkind_malloc(MEMKIND_REGULAR, 512);
    ASSERT_NE(nullptr, ptr);
    struct memkind *temp_kind = memkind_detect_kind(ptr);
    ASSERT_EQ(MEMKIND_REGULAR, temp_kind);
    memkind_free(MEMKIND_REGULAR, ptr);
}

TEST_F(MemkindDetectKindTests, test_TC_MEMKIND_DetectDefaultSmallSizeKind)
{
    void *ptr = memkind_malloc(MEMKIND_DEFAULT, 512);
    ASSERT_NE(nullptr, ptr);
    struct memkind *temp_kind = memkind_detect_kind(ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, temp_kind);
    memkind_free(MEMKIND_DEFAULT, ptr);
}

TEST_F(MemkindDetectKindTests, test_TC_MEMKIND_DetectDefaultBigSizeKind)
{
    void *ptr = memkind_malloc(MEMKIND_DEFAULT, 8 * MB);
    ASSERT_NE(nullptr, ptr);
    struct memkind *temp_kind = memkind_detect_kind(ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, temp_kind);
    memkind_free(MEMKIND_DEFAULT, ptr);
}

TEST_F(MemkindDetectKindTests, test_TC_MEMKIND_DetectKindMixKind)
{
    const size_t alloc_size = 512;
    memkind_t pmem_kind_temp_1 = nullptr;
    memkind_t pmem_kind_temp_2 = nullptr;
    struct memkind *temp_kind = nullptr;
    void *ptr_pmem_1 = nullptr;
    void *ptr_pmem_2 = nullptr;
    void *ptr_regular = nullptr;
    void *ptr_default = nullptr;

    int err = memkind_create_pmem(PMEM_DIR, 0, &pmem_kind_temp_1);
    ASSERT_EQ(0, err);
    ASSERT_NE(nullptr, pmem_kind_temp_1);

    err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_kind_temp_2);
    ASSERT_EQ(0, err);
    ASSERT_NE(nullptr, pmem_kind_temp_2);

    ptr_pmem_1 = memkind_malloc(pmem_kind_temp_1, alloc_size);
    ASSERT_NE(nullptr, ptr_pmem_1);

    ptr_pmem_2 = memkind_malloc(pmem_kind_temp_2, alloc_size);
    ASSERT_NE(nullptr, ptr_pmem_2);

    ptr_default = memkind_malloc(MEMKIND_DEFAULT, alloc_size);
    ASSERT_NE(nullptr, ptr_default);

    ptr_regular = memkind_malloc(MEMKIND_REGULAR, alloc_size);
    ASSERT_NE(nullptr, ptr_regular);

    temp_kind = memkind_detect_kind(ptr_default);
    ASSERT_EQ(MEMKIND_DEFAULT, temp_kind);

    temp_kind = memkind_detect_kind(ptr_pmem_1);
    ASSERT_EQ(pmem_kind_temp_1, temp_kind);

    temp_kind = memkind_detect_kind(ptr_regular);
    ASSERT_EQ(MEMKIND_REGULAR, temp_kind);

    temp_kind = memkind_detect_kind(ptr_pmem_2);
    ASSERT_EQ(pmem_kind_temp_2, temp_kind);

    memkind_free(pmem_kind_temp_2, ptr_pmem_2);
    memkind_free(pmem_kind_temp_1, ptr_pmem_1);
    memkind_free(MEMKIND_DEFAULT, ptr_default);
    memkind_free(MEMKIND_REGULAR, ptr_regular);

    err = memkind_destroy_kind(pmem_kind_temp_2);
    ASSERT_EQ(0, err);
    err = memkind_destroy_kind(pmem_kind_temp_1);
    ASSERT_EQ(0, err);
}
