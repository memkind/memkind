/*
 * Copyright (C) 2014 - 2016 Intel Corporation.
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

#include <memkind.h>

#include <fstream>
#include <algorithm>

#include "common.h"
#include "check.h"
#include "omp.h"
#include "trial_generator.h"

class StressTest : public TGTest
{
};

TEST_F(StressTest, TC_MEMKIND_HBW_TrialsTwoKind)
{
    tgen->generate_multi_app_stress(2,MEMKIND_MALLOC);
    tgen->run(num_bandwidth, bandwidth);
}

TEST_F(StressTest, TC_MEMKIND_HBW_TrialsAllKind)
{
    tgen->generate_multi_app_stress(6,MEMKIND_MALLOC);
    tgen->run(num_bandwidth, bandwidth);
}

TEST_F(StressTest, TC_MEMKIND_HBW_TrialsTwoKindDatacheck)
{
    tgen->generate_multi_app_stress(2,MEMKIND_DATACHECK);
    tgen->run(num_bandwidth, bandwidth);
}

TEST_F(StressTest, TC_MEMKIND_HBW_TrialsAllKindDatacheck)
{
    tgen->generate_multi_app_stress(6,MEMKIND_DATACHECK);
    tgen->run(num_bandwidth, bandwidth);
}
