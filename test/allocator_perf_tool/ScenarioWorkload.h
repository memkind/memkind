// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once
#include "Allocator.hpp"
#include "AllocatorFactory.hpp"
#include "FunctionCalls.hpp"
#include "Workload.hpp"

#include <string.h>

class ScenarioWorkload: public Workload
{
public:
    ScenarioWorkload(VectorIterator<Allocator *> *a, VectorIterator<size_t> *as,
                     VectorIterator<int> *fc);
    ~ScenarioWorkload(void);

    double get_time_costs();

    const std::vector<memory_operation> &get_allocations_info() const
    {
        return allocations;
    }

    bool run();

    memory_operation *get_allocated_memory();

    void enable_touch_memory_on_allocation(bool enable)
    {
        touch_memory_on_allocation = enable;
    }

    void post_allocation_check(const memory_operation &data);

private:
    AllocatorFactory allocator_factory;
    std::vector<memory_operation> allocations;

    bool touch_memory_on_allocation;

    VectorIterator<int> *func_calls;
    VectorIterator<size_t> *alloc_sizes;
    VectorIterator<Allocator *> *allocators;
};
