/*
 * Copyright (C) 2014 Intel Corperation.
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

#include <fstream>
#include <algorithm>
#include <numa.h>
#include <errno.h>
#include "common.h"
#include "check.h"
#include "omp.h"
#include "numakind.h"

#include "trial_generator.h"


class NegativeTest: public :: testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}

};


TEST_F(NegativeTest, ErrorUnavailable)
{
    int ret = 0;
    int numakind_flags;
    unsigned long maxnodes = NUMA_NUM_NODES;
    nodemask_t nodemask;
    numakind_error_t err = NUMAKIND_ERROR_UNAVAILABLE;
    ret = numakind_get_mmap_flags(-1,
                                  &numakind_flags);
    EXPECT_EQ(NUMAKIND_ERROR_UNAVAILABLE, ret);
    ret = numakind_get_nodemask(-1, nodemask.n, maxnodes);

    EXPECT_EQ(err, ret);
    ret = numakind_mbind(-1, NULL, 1024);

    EXPECT_EQ(err, ret);
}


TEST_F(NegativeTest, ErrorMbind)
{
    int ret = 0;
    numakind_error_t err = NUMAKIND_ERROR_MBIND;

    ret = numakind_mbind(NUMAKIND_HBW,
                         NULL,
                         1024);
    EXPECT_EQ(err, ret);
}

TEST_F(NegativeTest, ErrorMemalign)
{
    int ret = 0;
    void *ptr = NULL;
    numakind_error_t err = NUMAKIND_ERROR_MEMALIGN;

    ret = numakind_posix_memalign(NUMAKIND_DEFAULT,
                                  &ptr, 5,
                                  100);
    EXPECT_EQ(err, ret);
}

TEST_F(NegativeTest, ErrorAlignment)
{
    int ret = 0;
    void *ptr = NULL;
    numakind_error_t err = NUMAKIND_ERROR_ALIGNMENT;

    ret = numakind_posix_memalign(NUMAKIND_HBW,
                                  &ptr, 5,
                                  100);
    EXPECT_EQ(err, ret);
    EXPECT_EQ(errno, EINVAL);
}


TEST_F(NegativeTest, ErrorAllocM)
{
    int ret = 0;
    void *ptr = NULL;
    numakind_error_t err = NUMAKIND_ERROR_ALLOCM;

    ret = numakind_posix_memalign(NUMAKIND_HBW,
                                  &ptr,
                                  16,
                                  100*GB);
    EXPECT_EQ(err, ret);
}

TEST_F(NegativeTest, InvalidSizeMalloc)
{
    void *ptr = NULL;
    ptr = hbw_malloc(-1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);

    ptr = numakind_malloc(NUMAKIND_HBW, -1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);
}

TEST_F(NegativeTest, InvalidSizeCalloc)
{
    void *ptr = NULL;
    ptr = hbw_calloc(1, -1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);

    ptr = numakind_calloc(NUMAKIND_HBW, 1,
                          -1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);
}

TEST_F(NegativeTest, InvalidSizeRealloc)
{

    void *ptr = NULL;
    ptr = hbw_realloc(ptr, -1);
    ASSERT_TRUE(ptr==NULL);
    EXPECT_EQ(errno, ENOMEM);

    ptr = numakind_realloc(NUMAKIND_HBW,
                           ptr,
                           -1);
    ASSERT_TRUE(ptr==NULL);
    EXPECT_EQ(errno, ENOMEM);
}

TEST_F(NegativeTest, InvalidSizeMemalign)
{
    int ret = 0;
    void *ptr = NULL;
    numakind_error_t err = NUMAKIND_ERROR_INVALID;
    ret = hbw_allocate_memalign(&ptr,1,-1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);

    ret = numakind_posix_memalign(NUMAKIND_HBW,
                                  &ptr, 5, 100);
    err = NUMAKIND_ERROR_ALIGNMENT;
    EXPECT_EQ(err, ret);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, EINVAL);
}
