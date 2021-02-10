// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

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
    err =
        memkind_get_stat(MEMKIND_REGULAR, MEMKIND_STAT_TYPE_MAX_VALUE, &value);
    ASSERT_EQ(MEMKIND_ERROR_INVALID, err);
}

TEST_F(MemkindStatTests, test_TC_MEMKIND_DefaultKind)
{
    size_t value;
    int err = memkind_update_cached_stats();
    ASSERT_EQ(MEMKIND_SUCCESS, err);
    void *ptr = memkind_malloc(MEMKIND_DEFAULT, 100);
    for (int i = 0; i < MEMKIND_STAT_TYPE_MAX_VALUE; ++i) {
        err = memkind_get_stat(MEMKIND_DEFAULT,
                               static_cast<memkind_stat_type>(i), &value);
        ASSERT_EQ(MEMKIND_SUCCESS, err);
    }
    memkind_free(nullptr, ptr);
}

TEST_F(MemkindStatTests, test_TC_MEMKIND_EmptyRegularKind)
{
    size_t stat;
    int err = memkind_update_cached_stats();
    ASSERT_EQ(MEMKIND_SUCCESS, err);
    for (int i = 0; i < MEMKIND_STAT_TYPE_MAX_VALUE; ++i) {
        err = memkind_get_stat(MEMKIND_REGULAR,
                               static_cast<memkind_stat_type>(i), &stat);
        ASSERT_EQ(MEMKIND_SUCCESS, err);
        ASSERT_EQ(stat, 0U);
    }
}

TEST_F(MemkindStatTests, test_TC_MEMKIND_GetGlobal)
{
    size_t value;
    int err = memkind_update_cached_stats();
    ASSERT_EQ(MEMKIND_SUCCESS, err);
    for (int i = 0; i < MEMKIND_STAT_TYPE_MAX_VALUE; ++i) {
        int err = memkind_get_stat(nullptr, static_cast<memkind_stat_type>(i),
                                   &value);
        ASSERT_EQ(MEMKIND_SUCCESS, err);
    }
}
