// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind.h>
#include <test/proc_stat.h>

#include "common.h"

class BackgroundThreadsTests: public :: testing::Test
{
protected:
    void SetUp()
    {}

    void TearDown()
    {}

    ProcStat proc_stat;
};

TEST_F(BackgroundThreadsTests, test_TC_MEMKIND_BackgroundThreadsSwitch)
{
    const size_t alloc_size = 1 * MB;

    void *ptr = memkind_malloc(MEMKIND_REGULAR, alloc_size);

    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', alloc_size);

    unsigned old_threads_count = proc_stat.get_threads_count();
    bool state = true;
    ASSERT_TRUE(memkind_set_bg_threads(state) == 0); // enable bg threads
    unsigned new_threads_count = proc_stat.get_threads_count();
    ASSERT_GT(new_threads_count,
              old_threads_count); // check if there are more threads now
    state = 0;
    ASSERT_TRUE(memkind_set_bg_threads(state) == 0); // disable bg threads
    new_threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(old_threads_count, new_threads_count);

    memkind_free(MEMKIND_REGULAR, ptr);
}
