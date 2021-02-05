// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */

#include "allocator_perf_tool/HugePageOrganizer.hpp"
#include "common.h"
#include "hbwmalloc.h"
#include "memkind.h"
#include "trial_generator.h"

/* This set of tests are intended to test HBW_PAGESIZE_1GB and
 * HBW_PAGESIZE_1GB_STRICT flags.
 */
class GBPagesTestBindPolicy: public TGTest
{
protected:
    void run(int iterations, size_t alignment, bool psize_strict)
    {
        std::vector<void *> addr_to_free;
        hbw_set_policy(HBW_POLICY_BIND);
        EXPECT_EQ(HBW_POLICY_BIND, hbw_get_policy());

        for (int i = 0; i < iterations; i++) {
            void *ptr = NULL;
            int ret;
            if (psize_strict) {
                ret = hbw_posix_memalign_psize(&ptr, alignment, GB,
                                               HBW_PAGESIZE_1GB_STRICT);
            } else {
                ret = hbw_posix_memalign_psize(&ptr, alignment, GB,
                                               HBW_PAGESIZE_1GB);
            }
            ASSERT_EQ(0, ret);
            ASSERT_FALSE(ptr == NULL);
            ASSERT_EQ(0, hbw_verify_memory_region(ptr, GB, HBW_TOUCH_PAGES));
            addr_to_free.push_back(ptr);
        }

        for (int i = 0; i < iterations; i++) {
            hbw_free(addr_to_free[i]);
        }
    }

private:
    HugePageOrganizer huge_page_organizer = HugePageOrganizer(2250);
};

TEST_F(GBPagesTestBindPolicy,
       test_TC_MEMKIND_GBPages_ext_HBW_Misalign_Preferred_Bind_Strict_1GB)
{
    run(1, 2 * GB, true);
}

TEST_F(GBPagesTestBindPolicy,
       test_TC_MEMKIND_GBPages_ext_HBW_Memalign_Psize_Bind_1GB)
{
    run(1, GB, true);
}

/*
 * Below tests allocate GB pages incrementally.
 */

TEST_F(GBPagesTestBindPolicy,
       test_TC_MEMKIND_GBPages_ext_HBW_Memalign_Psize_Bind_2GB)
{
    run(2, GB, false);
}

TEST_F(GBPagesTestBindPolicy,
       test_TC_MEMKIND_GBPages_ext_HBW_Memalign_Psize_Strict_Bind_3GB)
{
    run(3, GB, true);
}
