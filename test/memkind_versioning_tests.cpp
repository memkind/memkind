// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */

#include "memkind.h"

#include "common.h"

/*
 * memkind versioning tests.
 */
class MemkindVersioningTests: public ::testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}

    const int max_version_value = 999;
};

// Test memkind_get_version().
TEST_F(MemkindVersioningTests, test_TC_MEMKIND_GetVersionFunc)
{
    int max_return_val = 1000000 * max_version_value +
        1000 * max_version_value + max_version_value;

    // version number > 0
    EXPECT_GT(memkind_get_version(), 0);

    // version number <= max_return_val
    EXPECT_LE(memkind_get_version(), max_return_val);
}
