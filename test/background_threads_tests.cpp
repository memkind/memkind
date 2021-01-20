// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/memkind_log.h>

#include "common.h"
#include "config.h"

#include <numa.h>

class BackgroundThreadsTests: public :: testing::Test
{
protected:
    void SetUp()
    {}

    void TearDown()
    {}

};

TEST_F(BackgroundThreadsTests, test_TC_MEMKIND_BackgroundThreadsEnable)
{
    unsigned cpus_count = numa_num_configured_cpus();

    for (unsigned i = 0; i < 10 * cpus_count; i++) {
		ASSERT_TRUE(memkind_malloc(MEMKIND_REGULAR, 1) != NULL);
	}

    bool enable = true;
    ASSERT_TRUE(memkind_set_background_threads(enable) == 0);
    enable = false;
    ASSERT_TRUE(memkind_set_background_threads(enable) == 0);
}