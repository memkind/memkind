/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
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

TEST_F(NegativeTest, InvalidSizeRealloc){

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
