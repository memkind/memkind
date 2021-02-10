// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <assert.h>
#include <cstdlib>
#include <vector>

#include "Configuration.hpp"
#include "FunctionCallsPerformanceTask.h"
#include "Task.hpp"

class TaskFactory
{
public:
    Task *create(TaskConf conf)
    {
        Task *task = NULL;
        task = new FunctionCallsPerformanceTask(conf);

        tasks.push_back(task);

        return task;
    }

    ~TaskFactory()
    {
        for (int i = 0; i < tasks.size(); i++) {
            delete tasks[i];
        }
    }

private:
    std::vector<Task *> tasks;
};
