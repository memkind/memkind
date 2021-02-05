// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#include "FunctionCallsPerformanceTask.h"

void FunctionCallsPerformanceTask::run()
{
    VectorIterator<size_t> allocation_sizes =
        AllocationSizes::generate_random_sizes(task_conf.allocation_sizes_conf,
                                               task_conf.seed);

    VectorIterator<int> func_calls =
        FunctionCalls::generate_random_allocator_func_calls(
            task_conf.n, task_conf.seed, task_conf.func_calls);

    AllocatorFactory allocator_types;
    VectorIterator<Allocator *> allocators_calls =
        allocator_types.generate_random_allocator_calls(
            task_conf.n, task_conf.seed, task_conf.allocators_types);

    ScenarioWorkload scenario_workload =
        ScenarioWorkload(&allocators_calls, &allocation_sizes, &func_calls);

    while (scenario_workload.run())
        ;

    results = scenario_workload.get_allocations_info();
}
