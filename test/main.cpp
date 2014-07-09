/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
 */

#include "common.h"
#include "hbwmalloc.h"

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    if (hbw_is_available() == 0) {
        std.cerr << "WARNING: hbw_is_available() == 0\n";
    }
    return RUN_ALL_TESTS();
}
