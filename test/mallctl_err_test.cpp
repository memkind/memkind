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
#include "../numakind.h"



class MallctlTest: public :: testing::Test
{

protected:
    void SetUp()
    {}
    
    void TearDown()
    {}

};

TEST_F(MallctlTest, ErrorMallctl)
{
    int ret = 0;
    int err = NUMAKIND_ERROR_MALLCTL;
    void *ptr = NULL;
    ret = numakind_posix_memalign(NUMAKIND_HBW,
				  &ptr, 16,
				  1024);

    EXPECT_EQ(err, ret);
}
