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
#include "../numakind_hbw.h"



class EnvTest: public :: testing::Test
{

protected:
    void SetUp()
    {}
    
    void TearDown()
    {}

};

TEST_F(EnvTest, ErrorEnviron)
{
    int ret = 0;
    int err = NUMAKIND_ERROR_ENVIRON;
    unsigned long *nodemask=NULL;
    NUMAKIND_BANDWIDTH_PATH=" ";
    system ("unset NUMAKIND_HBW_NODES");
    ret = numakind_hbw_get_nodemask (nodemask,
				     NUMA_NUM_NODES);
    EXPECT_EQ(err, ret);
}
