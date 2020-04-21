// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2018 Intel Corporation. */
#pragma once

#include "Task.hpp"
#include "AllocatorFactory.hpp"
#include "FunctionCalls.hpp"
#include "AllocationSizes.hpp"
#include "VectorIterator.hpp"
#include "ScenarioWorkload.h"
#include "Configuration.hpp"


class FunctionCallsPerformanceTask :
    public Task
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
