/*
 * Copyright (C) 2014 Intel Corporation.
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
#include <limits.h>
#include "common.h"
#include "check.h"
#include "omp.h"
#include "memkind.h"
#include "memkind_gbtlb.h"

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
    void *ret;
    void *err = (void *)(-1); /* MAP_FAILED */

    ret = memkind_partition_mmap(-1, NULL, 1024);
    EXPECT_EQ(err, ret);
}


TEST_F(NegativeTest, ErrorMbind)
{
    int ret = 0;
    int err = MEMKIND_ERROR_MBIND;

    ret = MEMKIND_HBW->ops->mbind(MEMKIND_HBW, NULL, 1024);
    EXPECT_EQ(err, ret);
}

TEST_F(NegativeTest, ErrorMemalign)
{
    int ret = 0;
    void *ptr = NULL;
    int err = EINVAL;

    errno = 0;
    ret = memkind_posix_memalign(MEMKIND_DEFAULT,
                                 &ptr, 5,
                                 100);
    EXPECT_EQ(err, ret);
    EXPECT_EQ(errno, 0);
}

TEST_F(NegativeTest, ErrorAlignment)
{
    int ret = 0;
    void *ptr = NULL;
    int err = EINVAL;

    errno = 0;
    ret = memkind_posix_memalign(MEMKIND_HBW,
                                 &ptr, 5,
                                 100);
    EXPECT_EQ(err, ret);
    EXPECT_EQ(errno, 0);
}


TEST_F(NegativeTest, ErrorAllocM)
{
    int ret = 0;
    void *ptr = NULL;
    int err = ENOMEM;

    errno = 0;
    ret = memkind_posix_memalign(MEMKIND_HBW,
                                 &ptr,
                                 16,
                                 100*GB);
    EXPECT_EQ(err, ret);
    EXPECT_EQ(errno, 0);
}

TEST_F(NegativeTest, InvalidSizeMalloc)
{
    void *ptr = NULL;
    ptr = hbw_malloc(-1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);

    ptr = memkind_malloc(MEMKIND_HBW, -1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);
}

TEST_F(NegativeTest, InvalidSizeCalloc)
{
    void *ptr = NULL;
    ptr = hbw_calloc(1, -1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);

    ptr = memkind_calloc(MEMKIND_HBW, 1,
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

    ptr = memkind_realloc(MEMKIND_HBW,
                          ptr,
                          -1);
    ASSERT_TRUE(ptr==NULL);
    EXPECT_EQ(errno, ENOMEM);
}

TEST_F(NegativeTest, InvalidSizeMemalign)
{
    int ret = 0;
    void *ptr = NULL;
    int err = ENOMEM;

    errno = 0;
    ret = hbw_posix_memalign(&ptr,4096,ULLONG_MAX);
    EXPECT_TRUE(ptr == NULL);
    EXPECT_EQ(ret, err);
    EXPECT_EQ(errno, 0);

    errno = 0;
    ret = memkind_posix_memalign(MEMKIND_HBW, &ptr, 4096, ULLONG_MAX);
    EXPECT_TRUE(ptr == NULL);
    EXPECT_EQ(err, ret);
    EXPECT_EQ(errno, 0);
}

TEST_F(NegativeTest, GBFailureTestMemalign)
{
    int ret = 0;
    void *ptr = NULL;
    int err = EINVAL;

    ret = hbw_posix_memalign_psize (&ptr, 1073741824, 1073741826,
                                    HBW_PAGESIZE_1GB_STRICT);
    EXPECT_EQ(ret, err);
    EXPECT_TRUE(ptr == NULL);
}

TEST_F(NegativeTest, RegularReallocWithMemalign)
{
    int ret = 0;
    void *ptr = NULL;

    ret = hbw_posix_memalign_psize (&ptr, 4096, 4096,
                                    HBW_PAGESIZE_4KB);
    ASSERT_TRUE(ptr != NULL);
    memset(ptr, 0, 4096);
    ptr = hbw_realloc (ptr, 8192);
    memset(ptr, 0, 8192);
    hbw_free(ptr);
}

TEST_F(NegativeTest, SetPolicyTest)
{
    hbw_set_policy(2);
    hbw_set_policy(1);
    hbw_set_policy(3);

}

TEST_F(NegativeTest, GBMemalignPsizeAlign)
{
    void *ptr = NULL;
    int ret = 0;
    int err = EINVAL;

    ret = hbw_posix_memalign_psize(&ptr, -1, 1024, HBW_PAGESIZE_1GB);
    EXPECT_EQ(err, ret);
}

TEST_F(NegativeTest, GBNullRealloc)
{
    void *ptr = NULL;
    ptr = memkind_gbtlb_realloc(MEMKIND_HBW_GBTLB, NULL, -1);
    EXPECT_TRUE(ptr == NULL);
}

TEST_F(NegativeTest, GBNullFree)
{
    memkind_free(MEMKIND_GBTLB,NULL);
}
