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



class TiedDistTest: public :: testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}

};

TEST_F(TiedDistTest, ErrorTiedDist)
{
    int ret = 0;
    int err = NUMAKIND_ERROR_TIEDISTANCE;
    unsigned long *nodemask=NULL;
    ret = numakind_hbw_get_nodemask (nodemask,
                                     NUMA_NUM_NODES);
    EXPECT_EQ(err, ret);
}
