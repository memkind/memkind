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

class MemkindStatTests: public ::testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindStatTests, test_TC_MEMKIND_StatUnknownType)
{
    size_t value;
    int err = memkind_get_stat(nullptr, MEMKIND_STAT_TYPE_MAX_VALUE, &value);
    ASSERT_EQ(MEMKIND_ERROR_INVALID, err);
    err = memkind_get_stat(MEMKIND_REGULAR, MEMKIND_STAT_TYPE_MAX_VALUE, &value);
    ASSERT_EQ(MEMKIND_ERROR_INVALID, err);
}

TEST_F(MemkindStatTests, test_TC_MEMKIND_DefaultKind)
{
    size_t value;
    int err = memkind_update_cached_stats();
    ASSERT_EQ(MEMKIND_SUCCESS, err);
    void *ptr = memkind_malloc(MEMKIND_DEFAULT, 100);
    for(int i = 0; i < MEMKIND_STAT_TYPE_MAX_VALUE; ++i) {
        err = memkind_get_stat(MEMKIND_DEFAULT, static_cast<memkind_stat_type>(i),
                               &value);
        ASSERT_EQ(MEMKIND_SUCCESS, err);
    }
    memkind_free(nullptr, ptr);
}

TEST_F(MemkindStatTests, test_TC_MEMKIND_EmptyRegularKind)
{
    size_t stat;
    int err = memkind_update_cached_stats();
    ASSERT_EQ(MEMKIND_SUCCESS, err);
    for(int i = 0; i < MEMKIND_STAT_TYPE_MAX_VALUE; ++i) {
        err = memkind_get_stat(MEMKIND_REGULAR, static_cast<memkind_stat_type>(i),
                               &stat);
        ASSERT_EQ(MEMKIND_SUCCESS, err);
        ASSERT_EQ(stat, 0U);
    }
}

TEST_F(MemkindStatTests, test_TC_MEMKIND_GetGlobal)
{
    size_t value;
    int err = memkind_update_cached_stats();
    ASSERT_EQ(MEMKIND_SUCCESS, err);
    for(int i = 0; i < MEMKIND_STAT_TYPE_MAX_VALUE; ++i) {
        int err = memkind_get_stat(nullptr, static_cast<memkind_stat_type>(i), &value);
        ASSERT_EQ(MEMKIND_SUCCESS, err);
    }
}
