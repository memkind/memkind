// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2021 Intel Corporation. */
#pragma once

#include "Allocator.hpp"

class Workload
{
public:
    virtual bool run() = 0;
    virtual ~Workload(void)
    {}

protected:
    Allocator *allocator;
};
