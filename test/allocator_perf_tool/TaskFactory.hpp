// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2018 Intel Corporation. */
#pragma once

#include <cstdlib>
#include <vector>
#include <assert.h>

#include "Task.hpp"
#include "FunctionCallsPerformanceTask.h"
#include "Configuration.hpp"


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
        for (int i=0; i<tasks.size(); i++) {
            delete tasks[i];
        }
    }

private:
    std::vector<Task *> tasks;
};
