// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind.h>
#include <test/proc_stat.h>

#include "common.h"

#include <numa.h>

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
    unsigned cpus_count = numa_num_configured_cpus();
    std::set<void *> allocations;
    const size_t alloc_size = 1 * MB;

    void *ptr = memkind_malloc(MEMKIND_REGULAR, alloc_size);

    for (unsigned i = 0; i < 10 * cpus_count; i++) {
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);
    }

    unsigned old_threads_count = proc_stat.get_threads_count();
    int enable = 1;
    int *enable_ptr = &enable;
    size_t sz = sizeof(bool);
    ASSERT_TRUE(memkind_set_bg_threads(NULL, sz, enable_ptr, sz) == 0); // enable bg threads
    unsigned new_threads_count = proc_stat.get_threads_count();
    ASSERT_GT(new_threads_count, old_threads_count); // check if there are more threads now
    enable = 0;
    ASSERT_TRUE(memkind_set_bg_threads(NULL, sz, enable_ptr, sz) == 0); // disable bg threads
    new_threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(old_threads_count, new_threads_count);

    for (auto const &ptr: allocations) {
        memkind_free(MEMKIND_REGULAR, ptr);
    }
}