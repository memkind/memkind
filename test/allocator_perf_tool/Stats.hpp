// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <map>
#include <vector>

#include "Allocation_info.hpp"
#include "Configuration.hpp"
#include "FunctionCalls.hpp"

class MethodStats
{
public:
    MethodStats()
    {
        total_time = 0.0;
        average_time = 0;
        allocation_size = 0;
        samples_num = 0;
    }

    double total_time;
    double average_time;
    unsigned samples_num;
    size_t allocation_size;
};

class TimeStats
{
public:
    TimeStats()
    {
        allocated = 0;
        deallocated = 0;
    }

    std::map<unsigned, std::map<unsigned, MethodStats>> stats;

    TimeStats &operator+=(const std::vector<memory_operation> &data)
    {
        for (size_t i = 0; i < data.size(); i++) {
            memory_operation tmp = data[i];
            MethodStats &method_stats =
                stats[tmp.allocator_type][tmp.allocation_method];
            method_stats.allocation_size += tmp.size_of_allocation;
            method_stats.total_time += tmp.total_time;
            method_stats.samples_num++;

            // Update average.
            double total_time = method_stats.total_time;
            double samples_num = method_stats.samples_num;
            double avg = total_time / samples_num;
            method_stats.average_time = avg;

            if (tmp.allocation_method != FunctionCalls::FREE) {
                allocated += tmp.size_of_allocation;
            } else {
                deallocated += tmp.size_of_allocation;
            }
        }

        return *this;
    }

    size_t get_allocated() const
    {
        return allocated;
    }
    size_t get_deallocated() const
    {
        return deallocated;
    }

private:
    size_t allocated;
    size_t deallocated;
};
