/*
 * Copyright (C) 2015 - 2016 Intel Corporation.
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
class AllocateToMaxStressTests: public :: testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}

    //Check true allocation errors over all iterations.
    //Return iteration number (>0) when error occurs, or zero
    int check_allocation_errors(std::vector<iteration_result>& results, const TaskConf& task_conf)
    {
        for (size_t i=0; i<results.size(); i++)
        {
            //Check if test ends with allocation error when reserved unallocated limit is enabled.
           if(results[i].is_allocation_error
                && task_conf.allocation_sizes_conf.reserved_unallocated)
           {
                return i+1;
           }
        }

        return 0;
    }

};

//Allocate memory to max using MEMKIND_HBW kind.
//NOTE: Allocated memory is limited (allocated_memory = total_free - reserved_unallocated).
TEST_F(AllocateToMaxStressTests, TC_ALLOCATE_TO_MAX_MEMKIND_HBW)
{
    unsigned allocator_id = AllocatorTypes::MEMKIND_HBW;
    RecordProperty("kind", AllocatorTypes::allocator_name(allocator_id));

    unsigned operations = 10000;
    unsigned size = 2048;
    RecordProperty("memory_operations", operations);
    RecordProperty("size", size);

    TaskConf task_conf = {
        operations, //number of memory operations
        {
            operations, //number of memory operations
            15, //reserved unallocated
            size, //no random sizes.
            size
        },
        TypesConf(FunctionCalls::MALLOC), //enable allocator function call
        TypesConf(allocator_id), //enable allocator
        11, //random seed
        false, //disable csv logging
        true //check memory availability
    };

    TimerSysTime timer;
    timer.start();

    //Execute test iterations.
    std::vector<iteration_result> results = StressIncreaseToMax::execute_test_iterations(task_conf, 1);

    float elapsed_time = timer.getElapsedTime();
    RecordProperty("elapsed_time", elapsed_time);

    //Check if the execution time is greater than minimum.
    EXPECT_GE(elapsed_time, 1.0);

    //Check finish status.
    EXPECT_EQ(check_allocation_errors(results, task_conf), 0);
}

//Allocate memory to max using MEMKIND_INTERLEAVE kind.
//NOTE: Allocated memory is limited (allocated_memory = total_free - reserved_unallocated).
TEST_F(AllocateToMaxStressTests, TC_ALLOCATE_TO_MAX_MEMKIND_INTERLEAVE)
{
    unsigned allocator_id = AllocatorTypes::MEMKIND_INTERLEAVE;
    RecordProperty("kind", AllocatorTypes::allocator_name(allocator_id));

    unsigned operations = 1000;
    unsigned size = 1048576; //1MB
    RecordProperty("memory_operations", operations);
    RecordProperty("size", size);

    TaskConf task_conf = {
        operations, //number of memory operations
        {
            operations, //number of memory operations
            15000, //reserved unallocated (MB)
            size, //no random sizes.
            size
        },
        TypesConf(FunctionCalls::MALLOC), //enable allocator function call
        TypesConf(allocator_id), //enable allocator
        11, //random seed
        false, //disable csv logging
        true //check memory availability
    };

    TimerSysTime timer;
    timer.start();

    //Execute test iterations.
    std::vector<iteration_result> results = StressIncreaseToMax::execute_test_iterations(task_conf, 1);

    float elapsed_time = timer.getElapsedTime();
    RecordProperty("elapsed_time", elapsed_time);

    //Check if the execution time is greater than minimum.
    EXPECT_GE(elapsed_time, 1.0);

    //Check finish status.
    EXPECT_EQ(check_allocation_errors(results, task_conf), 0);
}
