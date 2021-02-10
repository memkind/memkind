// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <vector>

#include "Allocation_info.hpp"
#include "Runnable.hpp"

class Task: public Runnable
{
public:
    virtual ~Task()
    {}

    virtual std::vector<memory_operation> get_results() = 0;
};
