/*
* Copyright (C) 2015 - 2018 Intel Corporation.
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

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <vector>

#include "Configuration.hpp"
#include "AllocatorFactory.hpp"
#include "TaskFactory.hpp"
#include "Task.hpp"
#include "ConsoleLog.hpp"
#include "Stats.hpp"
#include "Thread.hpp"
#include "Tests.hpp"
#include "CommandLine.hpp"
#include "FunctionCallsPerformanceTask.h"
#include "StressIncreaseToMax.h"

/*
Command line description.
Syntax:
	key=value
Options:
	- 'test' - specify the test case. This option can be used with the following values: 'calls', 'all' or 'self',
	where:
		'calls' - function calls performance test,
		'all' - execute both above ('footprint' and 'calls') tests,
		'self' - execute self tests
		's1' - stress tests
		(perform allocations until the maximum amount of allocated memory has been reached, than frees allocated memory.
		If the time interval has not been exceed, than repeat the test),
	- 'operations' - the number of memory operations per thread
	- 'size_from' - lower bound for the random sizes of allocation
	- 'size_to' - upper bound for the random sizes of allocation
	- 'seed' - random seed
	- 'threads_num' - the number of threads per test case
	- 'time' - minimum execution time interval
	- 'kind' - the kind to test
	- 'csv_log' - if 'true' then log to csv file memory operations and statistics
	- 'call' specify the allocation function call. This option can be used with the following values: 'malloc' (default), 'calloc', 'realloc',
	- 'requested_memory_limit' test stops when the requested memory limit has been reached
* - maximum of available memory in OS, or maximum memory based 'operations' parameter
Example:
1. Performance test:
./perf_tool test=all operations=1000 size_from=32 size_to=20480 seed=11 threads_num=200
2. Stress test
./perf_tool test=s1 time=120 kind=MEMKIND_HBW size_from=1048576 csv_log=true requested_memory_limit=1048576
*/

int main(int argc, char* argv[])
{
    unsigned mem_operations_num = 1000;
    size_t size_from = 32, size_to = 2048*1024;
    unsigned seed = 11;
    //should be at least one
    size_t threads_number = 10;

    CommandLine cmd_line(argc, argv);

    if((argc >= 1) && cmd_line.is_option_set("test", "self")) {
        execute_self_tests();
        getchar();
    }

    cmd_line.parse_with_strtol("operations", mem_operations_num);
    cmd_line.parse_with_strtol("size_from", size_from);
    cmd_line.parse_with_strtol("size_to", size_to);
    cmd_line.parse_with_strtol("seed", seed);
    cmd_line.parse_with_strtol("threads_num", threads_number);

    bool is_csv_log_enabled = cmd_line.is_option_set("csv_log", "true");

    //Heap Manager initialization
    std::vector<AllocatorFactory::initialization_stat> stats =
        AllocatorFactory().initialization_test();

    if(!cmd_line.is_option_set("print_init_stats", "false")) {
        printf("\nInitialization overhead:\n");
        for (int i=0; i<stats.size(); i++) {
            AllocatorFactory::initialization_stat stat = stats[i];
            printf("%32s : time=%7.7f.s, ref_delta_time=%15f, node0=%10fMB, node1=%7.7fMB\n",
                   AllocatorTypes::allocator_name(stat.allocator_type).c_str(),
                   stat.total_time,
                   stat.ref_delta_time,
                   stat.memory_overhead[0],
                   stat.memory_overhead[1]);
        }
    }

    //Stress test by repeatedly increasing memory (to maximum), until given time interval has been exceed.
    if(cmd_line.is_option_set("test", "s1")) {
        printf("Stress test (StressIncreaseToMax) start. \n");

        if(!cmd_line.is_option_present("operations"))
            mem_operations_num = 1000000;

        unsigned time = 120; //Default time interval.
        cmd_line.parse_with_strtol("time", time);

        size_t requested_memory_limit = 1024*1024;
        cmd_line.parse_with_strtol("requested_memory_limit", requested_memory_limit);

        unsigned allocator = AllocatorTypes::MEMKIND_HBW;
        if(cmd_line.is_option_present("kind")) {
            //Enable memkind allocator and specify kind.
            allocator = AllocatorTypes::allocator_type(cmd_line.get_option_value("kind"));
        }
        TypesConf allocator_types;
        allocator_types.enable_type(allocator);

        TypesConf enable_func_calls;
        enable_func_calls.enable_type(FunctionCalls::MALLOC);

        TaskConf task_conf = {
            mem_operations_num,
            {
                mem_operations_num,
                size_from, //No random sizes.
                size_from
            },
            enable_func_calls,
            allocator_types,
            11,
            is_csv_log_enabled,
        };

        StressIncreaseToMax::execute_test_iterations(task_conf, time,
                                                     requested_memory_limit);
        return 0;
    }

    printf("\nTest configuration: \n");
    printf("\t memory operations per thread = %u \n", mem_operations_num);
    printf("\t seed = %d\n", seed);
    printf("\t number of threads = %zu\n", threads_number);
    printf("\t size from-to = %zu-%zu\n\n", size_from, size_to);

    assert(size_from <= size_to);


    TypesConf func_calls;
    func_calls.enable_type(FunctionCalls::FREE);

    if(cmd_line.is_option_present("call")) {
        //Enable heap manager function call.
        func_calls.enable_type(FunctionCalls::function_type(
                                   cmd_line.get_option_value("call")));
    } else {
        func_calls.enable_type(FunctionCalls::MALLOC);
    }

    TypesConf allocator_types;
    if(cmd_line.is_option_present("allocator")) {
        allocator_types.enable_type(AllocatorTypes::allocator_type(
                                        cmd_line.get_option_value("allocator")));
    } else {
        for(unsigned i = 0; i <= AllocatorTypes::MEMKIND_HBW_PREFERRED; i++) {
            allocator_types.enable_type(i);
        }
    }

    TaskConf conf = {
        mem_operations_num, //number memory operations
        {
            mem_operations_num, //number of memory operations
            size_from, //min. size of single allocation
            size_to //max. size of single allocatioion
        },
        func_calls, //enable function calls
        allocator_types, //enable allocators
        seed, //random seed
        is_csv_log_enabled,
    };

    //Function calls test
    if(cmd_line.is_option_set("test", "calls") ||
       cmd_line.is_option_set("test", "all")) {
        TaskFactory task_factory;
        std::vector<Thread*> threads;
        std::vector<Task*> tasks;

        for (int i=0; i<threads_number; i++) {
            FunctionCallsPerformanceTask* task = static_cast<FunctionCallsPerformanceTask*>(
                                                     task_factory.create(conf)
                                                 );
            tasks.push_back(task);
            threads.push_back(new Thread(task));
            conf.seed += 1;
        }

        ThreadsManager threads_manager(threads);
        threads_manager.start();
        threads_manager.barrier();

        TimeStats stats;
        for (int i=0; i<tasks.size(); i++) {
            stats += tasks[i]->get_results();
        }

        ConsoleLog::print_table(stats);
        ConsoleLog::print_requested_memory(stats, "func. calls test");

        threads_manager.release();
    }

    return 0;
}
