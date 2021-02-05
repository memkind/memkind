// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include "Configuration.hpp"

#include "AllocationSizes.hpp"
#include "AllocatorFactory.hpp"
#include "CSVLogger.hpp"
#include "FunctionCalls.hpp"
#include "Numastat.hpp"
#include "ScenarioWorkload.h"
#include "Stats.hpp"
#include "Task.hpp"
#include "TimerSysTime.hpp"
#include "VectorIterator.hpp"

#include <vector>

struct iteration_result {
    bool has_next_memory_operation;
    bool is_allocation_error;
};

class StressIncreaseToMax: public Task
{
public:
    StressIncreaseToMax(const TaskConf &conf, size_t requested_memory_limit)
        : task_conf(conf), req_mem_limit(requested_memory_limit)
    {}

    void run();

    // Return memory operations from the last run.
    std::vector<memory_operation> get_results()
    {
        return results;
    }
    iteration_result get_test_status()
    {
        return test_status;
    }

    static std::vector<iteration_result>
    execute_test_iterations(const TaskConf &task_conf, unsigned time,
                            size_t requested_memory_limit);

private:
    size_t req_mem_limit;
    ScenarioWorkload *scenario_workload;
    std::vector<memory_operation> results;
    const TaskConf &task_conf;

    // Test status
    iteration_result test_status;
};
