// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2018 Intel Corporation. */
#pragma once

#include <vector>

#include "Runnable.hpp"
#include "Allocation_info.hpp"

class Task
    : public Runnable
{
public:
    virtual ~Task() {}

    virtual std::vector<memory_operation> get_results() = 0;
};

