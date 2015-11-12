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

#include "StressIncreaseToMax.h"


void StressIncreaseToMax::run()
{
	//Generate constant allocation sizes.
	VectorIterator<size_t> allocation_sizes = AllocationSizes::generate_random_sizes(task_conf.allocation_sizes_conf, task_conf.seed);
	//Generate only mallocs.
	VectorIterator<int> func_calls = FunctionCalls::generate_random_allocator_func_calls(task_conf.n, task_conf.seed, task_conf.func_calls);

	unsigned type;
	for (type = 0; type < AllocatorTypes::NUM_OF_ALLOCATOR_TYPES; type++)
	{
		if(task_conf.allocators_types.is_enabled(type))
			break; //Assume that there is only one type.
	}

	AllocatorFactory allocator_factory;
	VectorIterator<Allocator*> allocators_calls = allocator_factory.generate_const_allocator_calls(type, task_conf.n, task_conf.seed);

	ScenarioWorkload scenario_workload(
		&allocators_calls,
		&allocation_sizes,
		&func_calls
	);

	scenario_workload.enable_touch_memory_on_allocation(true);

	bool has_next;
	float allocated_memory = Numastat::get_total_memory(1);
	size_t total_memory = 0;
	size_t free_memory = 0;
	bool is_memory_available = true;
	bool is_allocation_error = false;
	int numa_node;


	std::ofstream csv_file;

	if(task_conf.is_csv_log_enabled)
	{
		csv_file.open("stress_test_increase_to_max_current_itr_log.csv");
		csv::Row row;
		row.append("Iteration");
		row.append("Size of allocation");
		row.append("Numa node");
		row.append("Is memory available");
		row.append("Free (MB)");
		row.append("Allocated (MB)");
		csv_file << row.export_row();
	}

	while (is_memory_available && !is_allocation_error && (has_next = scenario_workload.run()))
	{
		memory_operation data = scenario_workload.get_allocations_info().back();
		is_allocation_error = (data.error_code == ENOMEM) && (data.ptr == NULL);
		numa_node = get_numa_node(data.ptr);

		if(task_conf.check_memory_availability)
		{
			memkind_get_size(AllocatorFactory::get_kind_by_type(type), &total_memory, &free_memory);
			allocated_memory = Numastat::get_total_memory(numa_node);
			is_memory_available = ((convert_bytes_to_mb(task_conf.allocation_sizes_conf.size_from) + task_conf.allocation_sizes_conf.reserved_unallocated) < convert_bytes_to_mb(free_memory));
		}

		if(task_conf.is_csv_log_enabled)
		{
			csv::Row row;
			row.append(scenario_workload.get_allocations_info().size());
			row.append(convert_bytes_to_mb(data.size_of_allocation));
			row.append(numa_node);
			row.append(is_memory_available);
			row.append(convert_bytes_to_mb(free_memory));
			row.append(allocated_memory);
			csv_file << row.export_row();
		}
	}

	if(!has_next) printf("\nWARN: Too few memory operations. \n");
	if(is_allocation_error) printf("\nWARN: Allocation error. \n");

	results = scenario_workload.get_allocations_info();

	csv_file.close();
}

void StressIncreaseToMax::execute_test_iterations(TaskConf task_conf, unsigned time)
{
		TimerSysTime timer;
		unsigned itr = 0;

		std::ofstream csv_file;

		csv::Row row;
		row.append("Iteration");
		row.append("Allocated memory (MB)");
		row.append("Elapsed time (seconds)");
		if(task_conf.is_csv_log_enabled)
		{
			csv_file.open("stress_test_increase_to_max.csv");
			csv_file << row.export_row();
		}
		printf("%s", row.export_row().c_str());

		timer.start();

		while (timer.getElapsedTime() < time)
		{
			StressIncreaseToMax stress_test(task_conf);
			stress_test.run();
			float elapsed_time = timer.getElapsedTime();

			TimeStats stats;
			stats += stress_test.get_results();

			//Log every interation of StressIncreaseToMax test.
			csv::Row row;
			row.append(itr);
			row.append(convert_bytes_to_mb(stats.get_allocated()));
			row.append(elapsed_time);
			if(task_conf.is_csv_log_enabled)
			{
				csv_file << row.export_row();
			}
			printf("%s", row.export_row().c_str());
			fflush(stdout);

			itr++;
		}

		printf("\nStress test (StressIncreaseToMax) finish in time %f.\n",timer.getElapsedTime());

		csv_file.close();
}