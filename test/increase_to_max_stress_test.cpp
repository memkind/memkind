/*
 * Copyright (C) 2015 Intel Corporation.
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

#include "common.h"
#include "allocator_perf_tool/Configuration.hpp"
#include "allocator_perf_tool/StressIncreaseToMax.h"

//memkind stress and longevity tests using Allocatr Perf Tool.
class StressAndLongevityTests: public :: testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}

};

//Allocate memory to max using MEMKIND_HBW kind.
//NOTE: Allocated memory is limited (allocated_memory = total_free - reserved_unallocated).
TEST_F(StressAndLongevityTests, IncreaseToMaxMEMKIND_HBW)
{
    TypesConf allocator_types;
    allocator_types.enable_type(AllocatorTypes::MEMKIND_HBW);

    TypesConf enable_func_calls;
    enable_func_calls.enable_type(FunctionCalls::MALLOC);

    TaskConf task_conf = {
        10000, // number of memory operations
        {
            10000, // number of memory operations
            15, //reserved unallocated
            2048, //no random sizes.
            2048
        },
        enable_func_calls,
        allocator_types,
        11, //random seed
        false, //disable csv logging
        true //check memory availability
    };

    TimerSysTime timer;
    timer.start();

    //Execute test iterations.
    StressIncreaseToMax::execute_test_iterations(task_conf, 1);

    //Check if the execution time is greater than minimum.
    EXPECT_GE(timer.getElapsedTime(), 1.0);
}

//Allocate memory to max using MEMKIND_INTERLEAVE kind.
//NOTE: Allocated memory is limited (allocated_memory = total_free - reserved_unallocated).
TEST_F(StressAndLongevityTests, IncreaseToMaxMEMKIND_INTERLEAVE)
{
    TypesConf allocator_types;
    allocator_types.enable_type(AllocatorTypes::MEMKIND_INTERLEAVE);

    TypesConf enable_func_calls;
    enable_func_calls.enable_type(FunctionCalls::MALLOC);

    size_t allocation_size = 1048576; //1MB

    TaskConf task_conf = {
        1000, // number of memory operations
        {
            1000, // number of memory operations
            15000, //reserved unallocated (MB)
            allocation_size, //no random sizes.
            allocation_size
        },
        enable_func_calls,
        allocator_types,
        11, //random seed
        false, //disable csv logging
        true //check memory availability
    };

    TimerSysTime timer;
    timer.start();

    //Execute test iterations.
    StressIncreaseToMax::execute_test_iterations(task_conf, 1);

    //Check if the execution time is greater than minimum.
    EXPECT_GE(timer.getElapsedTime(), 1.0);
}
