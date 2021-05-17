// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2021 Intel Corporation. */

#include "memkind_memtier.h"

#include "common.h"
#include "config.h"
#include "decorator_memtier_test.h"

class DecoratorMemtierTest: public ::testing::Test
{

protected:
    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

TEST(DecoratorMemtierTest, test_flag)
{
    bool check = false;
    #ifdef MEMTIER_DECORATION_ENABLED
        printf("Flag works\n");
        check = true;
    #endif
    ASSERT_TRUE(check == true);
}