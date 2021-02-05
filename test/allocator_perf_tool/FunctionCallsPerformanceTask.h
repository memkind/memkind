// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include "AllocatorFactory.hpp"

#include "AllocationSizes.hpp"
#include "Configuration.hpp"
#include "FunctionCalls.hpp"
#include "ScenarioWorkload.h"
#include "Task.hpp"
#include "VectorIterator.hpp"

class FunctionCallsPerformanceTask: public Task
{
public:
    FunctionCallsPerformanceTask(TaskConf conf)
    {
        task_conf = conf;
    }

    void run();

    std::vector<memory_operation> get_results()
    {
        return results;
    }

private:
    TaskConf task_conf;
    std::vector<memory_operation> results;
};
