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
//#define PRINT_LOG

#include <stdio.h>
#include <assert.h>
#include <map>
#include <iostream>
#include <vector>

#include "Configuration.hpp"
#include "AllocationSizes.hpp"
#include "AllocatorFactory.hpp"
#include "TaskFactory.hpp"
#include "Task.hpp"
#include "ConsoleLog.hpp"
#include "Stats.hpp"
#include "Thread.hpp"
#include "FootprintTask.h"
#include "Tests.hpp"
#include "MemoryFootprintStats.hpp"
#include "Numastat.hpp"
#include "CommandLine.hpp"
#include "FootprintSampling.h"
#include "FunctionCallsPerformanceTask.h"

/*
Command line description.
Syntax:
	key=value
Options:
	- 'test' - specify the test case. This option can be used with the following values: 'footprint', 'calls', 'all' or 'self',
	where:
		'footprint' - memory footprint test,
		'calls' - function calls performance test,
		'all' - execute both above ('footprint' and 'calls') tests,
		'self' - execute self tests
	- 'operations' - the number of operations per thread
	- 'size_from' - lower bound for the random sizes of allocation
	- 'size_to' - upper bound for the random sizes of allocation
	- 'seed' - random seed
	- 'threads_num' - the number of threads per test case
Example:
./perf_tool test=all operations=1000 size_from=32 size_to=20480 seed=11 threads_num=200
*/

int main(int argc, char* argv[])
{
	int mem_operations_num = 1000;
	size_t size_from = 32, size_to = 2048*1024;
	unsigned seed = 11;
	//should be at least one
	size_t threads_number = 10;
	bool footprint_test_enabled = false;
	bool calls_test_enabled = false;

	CommandLine cmd_line(argc, argv);

	if((argc >= 1) && cmd_line.is_option_set("test", "self"))
	{
		execute_self_tests();
		getchar();
	}

	footprint_test_enabled = cmd_line.is_option_set("test", "footprint");
	calls_test_enabled = cmd_line.is_option_set("test", "calls");

	if(cmd_line.is_option_set("test", "all"))
	{
		calls_test_enabled = true;
		footprint_test_enabled = true;
	}

	cmd_line.parse_with_strtol("operations", mem_operations_num);
	cmd_line.parse_with_strtol("size_from", size_from);
	cmd_line.parse_with_strtol("size_to", size_to);
	cmd_line.parse_with_strtol("seed", seed);
	cmd_line.parse_with_strtol("threads_num", threads_number);


	printf("Test configuration: \n");
	printf("\t memory operations per thread = %u \n", mem_operations_num);
	printf("\t seed = %d\n", seed);
	printf("\t number of threads = %u\n", threads_number);
	printf("\t size from-to = %u-%u\n\n", size_from, size_to);

	assert(size_from <= size_to);
#ifdef PRINT_LOG
	float min = convert_bytes_to_mb(size_from * N *  threads_number);
	float mid = convert_bytes_to_mb(((size_from + size_to) / 2.0) * N *  threads_number);
	float max = convert_bytes_to_mb(size_to* N *  threads_number);
	printf("Allocation bound: min: %f, mid: %f, max: %f\n",  min, mid, max);
#endif

	float before_init = Numastat::get_total_memory(0);
	//Ensure library initialization at the first call.
	memory_operation data = AllocatorFactory().initialize_allocator(AllocatorTypes::MEMKIND_HBW);
	float memkind_initalization_overhead = Numastat::get_total_memory(0) - before_init;
	printf("memkind HBW initialization overhead: %f MB/%d.s. \n", memkind_initalization_overhead, data.total_time);

	if(!calls_test_enabled && !footprint_test_enabled)
	{
		calls_test_enabled = true;
		footprint_test_enabled = true;
	}

	TypesConf func_calls;
	func_calls.enable_type(FunctionCalls::MALLOC);
	func_calls.enable_type(FunctionCalls::FREE);

	TaskConf conf = {
		mem_operations_num, //number memory operations
		{
			mem_operations_num, //number of memory operations
			size_from, //min. size of single allocation
			size_to //max. size of single allocatioion
		},
		func_calls, //enable function calls
		seed //random seed
	};

	//Footprint test
	if(footprint_test_enabled)
	{
		std::vector<Thread*> threads;
		std::vector<FootprintTask*> tasks;

		FootprintSampling sampling;
		Thread sampling_thread(&sampling);

		TaskFactory task_factory;

		for (int i=0; i<threads_number; i++)
		{
			FootprintTask* task = static_cast<FootprintTask*>(task_factory.create(TaskFactory::FOOTPRINT_TASK, conf));

			sampling.register_task(task);
			tasks.push_back(task);
			threads.push_back(new Thread(task));
		}

		ThreadsManager threads_manager(threads);
		threads_manager.start(); //Threads begins to execute tasks with footprint workload.
		sampling_thread.start(); //Start sampling in separated thread.
		threads_manager.barrier(); //Waiting until each thread has completed.
		sampling.stop(); //Stop sampling.
		sampling_thread.wait(); //Wait for the sampling thread.

		MemoryFootprintStats mem_footprint_stats = sampling.get_memory_footprint_stats();
		TimeStats stats;
		for (int i=0; i<tasks.size(); i++)
		{
			stats += tasks[i]->get_results();
		}

		ConsoleLog::print_stats(mem_footprint_stats);
		ConsoleLog::print_requested_memory(stats, "footprint test");

		threads_manager.release();
	}

	//Function calls test
	if(calls_test_enabled)
	{
		TaskFactory task_factory;

		std::vector<Thread*> threads;
		std::vector<Task*> tasks;

		for (int i=0; i<threads_number; i++)
		{
			FunctionCallsPerformanceTask* task = static_cast<FunctionCallsPerformanceTask*>(
				task_factory.create(TaskFactory::FUNCTION_CALLS_PERFORMANCE_TASK, conf)
				);
			tasks.push_back(task);
			threads.push_back(new Thread(task));
			conf.seed += 1;
		}

		ThreadsManager threads_manager(threads);
		threads_manager.start();
		threads_manager.barrier();

		TimeStats stats;
		for (int i=0; i<tasks.size(); i++)
		{
			stats += tasks[i]->get_results();
		}

		ConsoleLog::print_table(stats);
		ConsoleLog::print_requested_memory(stats, "func. calls test");

		threads_manager.release();
	}

	return 0;
}