// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include "memkind.h"
#include "memkind/internal/memkind_private.h"

#include "common.h"
#include "static_kinds_list.h"

/*
 * Set of tests for checking if static kinds meets non-trivial assumptions
 */

class StaticKindsTest: public ::testing::Test
{
public:
protected:
    void SetUp()
    {}
};

/*
 * Assumption: all static kinds should implement init_once operation
 * Reason:  init_once should perform memkind_register (and other initialization
 * if needed) we are also using that fact to optimize initialization on first
 * use (in memkind_malloc etc.)
 */
TEST_F(StaticKindsTest, test_TC_MEMKIND_STATIC_KINDS_INIT_ONCE)
{
    for (size_t i = 0;
         i < (sizeof(static_kinds_list) / sizeof(static_kinds_list[0])); i++) {
        ASSERT_TRUE(static_kinds_list[i]->ops->init_once != NULL)
            << static_kinds_list[i]->name
            << " does not implement init_once operation!";
    }
}
