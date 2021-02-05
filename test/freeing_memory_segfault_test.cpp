// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2020 Intel Corporation. */

#include "common.h"
#include "memkind.h"
#include <thread>

// This test reproduce segfault when using TBB library.
class FreeingMemorySegfault: public ::testing::Test
{
protected:
    void SetUp()
    {}
    void TearDown()
    {}
};

TEST_F(FreeingMemorySegfault,
       test_TC_MEMKIND_freeing_memory_after_thread_finish)
{
    void *ptr = nullptr;

    std::thread t([&] {
        ptr = memkind_malloc(MEMKIND_DEFAULT, 32);
        ASSERT_TRUE(ptr != NULL);
    });
    t.join();

    memkind_free(0, ptr);
    SUCCEED();
}
